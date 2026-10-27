#include "Hardware/Audio/SID.h"
#include "Hardware/Core/ISerializable.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <mutex>
#include <iostream>
#include <numbers>
#include <vector>

namespace Hardware {

SID::SID() {
    voices[0].prevOsc = &voices[2];
    voices[1].prevOsc = &voices[0];
    voices[2].prevOsc = &voices[1];
    Reset();
}

SID::~SID() { Close(); }

void SID::Close() {
    audioSink.Close();
}

void SID::Init(int sampleRate) {
    resampler.SetRates(SID_NATIVE_RATE, static_cast<double>(sampleRate));
    audioSink.Init(sampleRate);
}

void SID::SetCpuClockRate(double rate) {
    if (rate <= 0.0) {
        return;
    }

    std::lock_guard<std::mutex> lock(sidMutex);
    cpuClockRateInt = static_cast<uint64_t>(rate);
    if (cpuClockRateInt == 0) {
        cpuClockRateInt = 1000000;
    }
}

void SID::SetClockRate(double newClockRate) {
    SetCpuClockRate(newClockRate);
}

void SID::SyncDormantEnvelopes() {
    if (!isDormant) {
        return;
    }
    const uint64_t elapsed = clockCounter - dormantClockAnchor;
    if (elapsed > 0) {
        for (auto& voice : voices) {
            if (voice.env.ratePeriod > 0) {
                voice.env.rateCounter = static_cast<uint16_t>(
                    (static_cast<uint64_t>(voice.env.rateCounter) + elapsed) % voice.env.ratePeriod);
            }
        }
    }
    dormantClockAnchor = clockCounter;
}

void SID::UpdateDormantState() {
    const Oscillator* const v = voices.data();
    const bool voicesSilent =
        (v[0].frequency == 0 && (v[0].control & 0x01) == 0 && v[0].env.holdZero && v[0].env.level == 0) &&
        (v[1].frequency == 0 && (v[1].control & 0x01) == 0 && v[1].env.holdZero && v[1].env.level == 0) &&
        (v[2].frequency == 0 && (v[2].control & 0x01) == 0 && v[2].env.holdZero && v[2].env.level == 0);
    const bool recording = audioSink.IsRecording();
    const bool canBeDormant = voicesSilent && !profilingEnabled && !recording && !audioSink.HasAudioCallback() &&
                              (model != SIDModel::MOS6581 || (volumeRegister & 0x0F) == 0) &&
                              filter.IsSilent();
    if (canBeDormant && !isDormant) {
        dormantClockAnchor = clockCounter;
    } else if (!canBeDormant && isDormant) {
        SyncDormantEnvelopes();
    }
    isDormant = canBeDormant;
}

void SID::Reset() {
    std::lock_guard<std::mutex> lock(sidMutex);
    registers.fill(0);
    for (auto& voice : voices) {
        voice.accumulator = 0;
        voice.frequency = 0;
        voice.pulseWidth = 0;
        voice.control = 0;
        voice.noiseShift = 0x7FFFF8;
        voice.env.state = ADSREnvelope::RELEASE;
        voice.env.level = 0;
        voice.env.rateCounter = 0;
        voice.env.ratePeriod = RATE_COUNTER_PERIODS[0];
        voice.env.exponentialCounter = 0;
        voice.env.exponentialCounterPeriod = 1;
        voice.env.holdZero = true;
        voice.env.attackRate = 0;
        voice.env.decayRate = 0;
        voice.env.sustainLevel = 0;
        voice.env.releaseRate = 0;
        voice.oscOutput = 0;
        voice.adsrBugMonitor.Reset();
    }
    volumeRegister = 0;
    filter.Reset();
    clockCounter = 0;
    dormantClockAnchor = 0;
    sidCycleAccumulator = 0;
    isDormant = false;
    resampler.Clear();
    trackerLogger.Reset();
    audioSink.Reset();
    UpdateDormantState();
}

void SID::SetModel(SIDModel newModel) {
    std::lock_guard<std::mutex> lock(sidMutex);
    this->model = newModel;
    filter.filterCoefficientsDirty = true;
    UpdateDormantState();
}

SIDModel SID::GetModel() const {
    return model;
}

void SID::EnableSound(bool enable) {
    std::lock_guard<std::mutex> lock(sidMutex);
    
    if (!soundEnabled && enable) {
        // Clear old state before enabling
        resampler.Clear();
        filter.dcBlockerState = 0.0;
        filter.dcBlockerPrevIn = 0.0;
        filter.filterCoefficientsDirty = true;
        audioSink.ClearAudioStream();
    }
    
    soundEnabled = enable;
    if (soundEnabled && !emulationPaused) {
        audioSink.OnResumePlayback(soundEnabled);
    }

    UpdateAudioState();
}

void SID::SetEmulationPaused(bool paused) {
    std::lock_guard<std::mutex> lock(sidMutex);

    if (emulationPaused == paused) {
        return;
    }
    
    if (emulationPaused && !paused) {
        // Resuming from pause: clear stale data to prevent audio pop
        resampler.Clear();
        filter.dcBlockerState = 0.0;
        filter.dcBlockerPrevIn = 0.0;
        audioSink.ClearAudioStream();
    }
    
    emulationPaused = paused;
    if (!emulationPaused && soundEnabled) {
        audioSink.OnResumePlayback(soundEnabled);
    }
    UpdateAudioState();
}

void SID::UpdateAudioState() {
    audioSink.UpdateAudioState(soundEnabled, emulationPaused);
}

Byte SID::Read(Word addr) {
    uint8_t reg = addr & 0x1F;
    if (reg == 0x19 || reg == 0x1A) return 0xFF; // Paddle X/Y
    if (reg == 0x1B) return voice3OscOutput;
    if (reg == 0x1C) return voice3EnvOutput;
    return registers.at(reg);
}

void SID::Write(Word addr, Byte data) {
    SyncDormantEnvelopes();
    uint8_t reg = addr & 0x1F;
    uint8_t* const regs = registers.data();
    regs[reg] = data;

    trackerLogger.LogWrite(reg, data, emulationPaused);

    if (reg < 0x15) {
        const std::size_t voiceIdx = reg / 7U;
        const std::size_t regOffset = voiceIdx * 7U;
        Oscillator& osc = *(voices.data() + voiceIdx);

        osc.frequency = static_cast<uint32_t>(regs[regOffset]) | (static_cast<uint32_t>(regs[regOffset + 1]) << 8);
        osc.pulseWidth = static_cast<uint16_t>(regs[regOffset + 2]) |
                         (static_cast<uint16_t>(regs[regOffset + 3] & 0x0F) << 8);
        osc.control = regs[regOffset + 4];

        const uint8_t attackDecay = regs[regOffset + 5];
        const uint8_t sustainRelease = regs[regOffset + 6];
        osc.env.targetAttackRate = (attackDecay >> 4) & 0xF;
        osc.env.targetDecayRate = attackDecay & 0xF;
        osc.env.targetSustainLevel = (sustainRelease >> 4) & 0xF;
        osc.env.targetReleaseRate = sustainRelease & 0xF;
        if (reg == regOffset + 5 || reg == regOffset + 6) {
            if (regs[regOffset + 5] == 0 && regs[regOffset + 6] == 0 && (osc.control & 0x01) == 0) {
                osc.adsrBugMonitor.MarkHardRestart();
            }
        }
        UpdateDormantState();
        return;
    }

    if (reg == 0x18) {
        volumeRegister = data & 0x0F;
        filterMode = data & 0xF0;
        filter.filterCoefficientsDirty = true;
    } else if (reg == 0x17) {
        filterFiltMask = data & 0x0F;
        filter.filterCoefficientsDirty = true;
    } else if (reg == 0x15 || reg == 0x16) {
        filter.filterCoefficientsDirty = true;
    }
    UpdateDormantState();
}

void SID::UpdateFilterCoefficients() {
    uint16_t fc = (registers.at(0x15) & 0x7) | (registers.at(0x16) << 3);
    uint8_t resFilt = registers.at(0x17);
    uint8_t res = (resFilt >> 4) & 0x0F;
    filter.UpdateFilterCoefficients(fc, res, model);
}

void SID::FlushAvailableOutput() {
    audioSink.FlushAvailableOutput(resampler, emulationPaused, soundEnabled);
}

template <bool Profiled>
void SID::ClockInternal() {
    clockCounter++;

    std::chrono::steady_clock::time_point oscStart;
    if constexpr (Profiled) {
        oscStart = std::chrono::steady_clock::now();
    }
    Oscillator* const v = voices.data();
    v[0].Next(model);
    v[1].Next(model);
    v[2].Next(model);
    if constexpr (Profiled) {
        profilingStats.oscillatorNanoseconds += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - oscStart)
                .count());
    }

    std::chrono::steady_clock::time_point envStart;
    if constexpr (Profiled) {
        envStart = std::chrono::steady_clock::now();
    }
    for (std::size_t i = 0; i < 3; ++i) {
        const bool newGate = (v[i].control & 0x01) != 0;
        const bool oldGate = v[i].env.gate;
        if (oldGate && !newGate) {
            const bool hardRestart = (v[i].env.targetAttackRate == 0 && v[i].env.targetDecayRate == 0 &&
                                      v[i].env.targetSustainLevel == 0 && v[i].env.targetReleaseRate == 0);
            v[i].adsrBugMonitor.OnGateOff(clockCounter, hardRestart);
        } else if (!oldGate && newGate) {
            const uint64_t twoFrames = (cpuClockRateInt > 0) ? ((2ULL * cpuClockRateInt) / 60ULL) : 33333ULL;
            const bool isAttackZero = (v[i].env.rateCounter > RATE_COUNTER_PERIODS.at(v[i].env.targetAttackRate));
            v[i].adsrBugMonitor.OnGateOn(clockCounter, twoFrames, isAttackZero);
        }
        v[i].env.Update(newGate);
    }
    if constexpr (Profiled) {
        profilingStats.envelopeNanoseconds += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - envStart)
                .count());
        ++profilingStats.clockCount;
    }

    voice3OscOutput = (v[2].oscOutput >> 4) & 0xFF;
    voice3EnvOutput = v[2].env.level;

    const uint16_t envOr = v[0].env.level | v[1].env.level | v[2].env.level;

    if (!soundEnabled) {
        if (envOr == 0) {
            filter.Reset();
            UpdateDormantState();
        }
        return;
    }

    std::chrono::steady_clock::time_point filterStart;
    if constexpr (Profiled) {
        filterStart = std::chrono::steady_clock::now();
    }

    if (filter.filterCoefficientsDirty) [[unlikely]] {
        UpdateFilterCoefficients();
    }

    const uint8_t currentVolume = volumeRegister & 0x0F;
    if (envOr == 0 && (model != SIDModel::MOS6581 || currentVolume == 0) &&
        filter.IsSilent()) [[likely]] {
        if constexpr (Profiled) {
            profilingStats.filterNanoseconds += static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - filterStart)
                    .count());
            const auto resamplerStart = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(sidMutex);
            resampler.PushSample(0.0);
            profilingStats.resamplerNanoseconds += static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - resamplerStart)
                    .count());
            const auto outputStart = std::chrono::steady_clock::now();
            if (resampler.HasOutput()) [[unlikely]] {
                FlushAvailableOutput();
            }
            profilingStats.outputNanoseconds += static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - outputStart)
                    .count());
        } else {
            std::lock_guard<std::mutex> lock(sidMutex);
            resampler.PushSample(0.0);
            if (resampler.HasOutput()) [[unlikely]] {
                FlushAvailableOutput();
            }
        }
        return;
    }

    double filteredInput = 0.0;
    double directOutput = 0.0;
    static constexpr double INV_2048 = 1.0 / 2048.0;
    static constexpr double INV_255 = 1.0 / 255.0;

    const uint8_t soloMask = voiceSoloMask.load(std::memory_order_relaxed);
    const uint8_t muteMask = voiceMuteMask.load(std::memory_order_relaxed);

    for (size_t idx = 0; idx < MAX_SID_VOICES; ++idx) {
        const bool active = (soloMask != 0 ? ((soloMask & (1U << idx)) != 0) : true) &&
                            ((muteMask & (1U << idx)) == 0);
        if (!active) {
            continue;
        }
        const uint8_t level = v[idx].env.level;
        if (level == 0) {
            continue;
        }
        double voiceSample = ((static_cast<double>(v[idx].oscOutput) * INV_2048) - 1.0) *
                             (static_cast<double>(level) * INV_255);
        if ((filterFiltMask & (1U << idx)) != 0) {
            filteredInput += voiceSample;
        } else if (idx != 2 || (filterMode & 0x80) == 0) {
            directOutput += voiceSample;
        }
    }

    double filteredMix = filter.Process(filteredInput, directOutput, filterMode, currentVolume, model, envOr == 0);
    if (std::abs(filteredMix) < 1e-15 && envOr == 0) {
        UpdateDormantState();
    }

    if constexpr (Profiled) {
        profilingStats.filterNanoseconds += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - filterStart)
                .count());

        const auto resamplerStart = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(sidMutex);
        resampler.PushSample(filteredMix);
        profilingStats.resamplerNanoseconds += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - resamplerStart)
                .count());

        const auto outputStart = std::chrono::steady_clock::now();
        if (resampler.HasOutput()) [[unlikely]] {
            FlushAvailableOutput();
        }
        profilingStats.outputNanoseconds += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - outputStart)
                .count());
    } else {
        std::lock_guard<std::mutex> lock(sidMutex);
        resampler.PushSample(filteredMix);
        if (resampler.HasOutput()) [[unlikely]] {
            FlushAvailableOutput();
        }
    }
}

void SID::ClockProfiled() {
    ClockInternal<true>();
}

void SID::ClockSlow() {
    trackerLogger.StepFrame(clockCounter, (cpuClockRateInt > 0) ? (cpuClockRateInt / 60) : 16666);

    if (profilingEnabled) [[unlikely]] {
        ClockProfiled();
        return;
    }
    ClockInternal<false>();
}

bool SID::SaveState(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(sidMutex);
    const_cast<SID*>(this)->SyncDormantEnvelopes();
    ISerializable::Serialize(out, registers);
    for (const auto& voice : voices) {
        voice.SaveState(out);
    }
    ISerializable::Serialize(out, volumeRegister);
    ISerializable::Serialize(out, soundEnabled);
    ISerializable::Serialize(out, emulationPaused);
    ISerializable::Serialize(out, filter.filterLow);
    ISerializable::Serialize(out, filter.filterBand);
    ISerializable::Serialize(out, filter.dcBlockerState);
    ISerializable::Serialize(out, filter.dcBlockerPrevIn);
    
    uint8_t mod = static_cast<uint8_t>(model);
    ISerializable::Serialize(out, mod);
    return out.good();
}

bool SID::LoadState(std::istream& inStream) {
    std::lock_guard<std::mutex> lock(sidMutex);
    ISerializable::Deserialize(inStream, registers);
    for (auto& voice : voices) {
        voice.LoadState(inStream);
    }
    ISerializable::Deserialize(inStream, volumeRegister);
    ISerializable::Deserialize(inStream, soundEnabled);
    ISerializable::Deserialize(inStream, emulationPaused);
    ISerializable::Deserialize(inStream, filter.filterLow);
    ISerializable::Deserialize(inStream, filter.filterBand);
    ISerializable::Deserialize(inStream, filter.dcBlockerState);
    ISerializable::Deserialize(inStream, filter.dcBlockerPrevIn);
    filter.filterCoefficientsDirty = true;
    
    uint8_t mod = 0;
    if (!inStream.eof()) {
        ISerializable::Deserialize(inStream, mod);
        model = static_cast<SIDModel>(mod);
    }
    filterFiltMask = registers[0x17] & 0x0F;
    filterMode = registers[0x18] & 0xF0;
    UpdateFilterCoefficients();
    isDormant = false;
    UpdateDormantState();
    UpdateAudioState();
    return inStream.good();
}

void SID::StartRecording(const std::string& filename) {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioSink.StartRecording(filename, emulationPaused, soundEnabled);
    UpdateDormantState();
}

void SID::StopRecording() {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioSink.StopRecording();
    UpdateDormantState();
}

bool SID::IsRecording() const {
    std::lock_guard<std::mutex> lock(sidMutex);
    return audioSink.IsRecording();
}

void SID::SetAudioCallback(std::function<void(const int16_t*, int)> callback) {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioSink.SetAudioCallback(std::move(callback));
    UpdateDormantState();
}

void SID::ClearAudioCallback() {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioSink.ClearAudioCallback();
    UpdateDormantState();
}

void SID::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
    UpdateDormantState();
}

void SID::ClearProfilingStats() {
    profilingStats = {};
}

SIDProfilingStats SID::GetProfilingStats() const {
    return profilingStats;
}

void SID::GetAudioSnapshot(std::span<float> outBuffer) const {
    audioSink.GetAudioSnapshot(outBuffer);
}

void SID::GetTrackerLog(std::vector<SIDTrackerFrame>& outLog) const {
    trackerLogger.GetTrackerLog(outLog);
}

void SID::ClearTrackerLog() {
    trackerLogger.Clear();
}

}  // namespace Hardware
