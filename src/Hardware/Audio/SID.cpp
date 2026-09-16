#include "Hardware/Audio/SID.h"
#include "Hardware/Core/ISerializable.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <span>
#include <cstring>
#include <iostream>
#include <vector>

#include "Hardware/Audio/SIDWaveTables.h"

namespace Hardware {

constexpr double SID_CLOCK = 1000000.0;

constexpr std::array<uint16_t, 16> RATE_COUNTER_PERIODS = {
      9,     32,     63,     95,    149,    220,    267,    313,
    392,    977,   1954,   3126,   3907,  11720,  19532,  31251
};

constexpr std::array<uint8_t, 16> SUSTAIN_LEVELS = {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

SID::SID() {
    voices[0].prevOsc = &voices[2];
    voices[1].prevOsc = &voices[0];
    voices[2].prevOsc = &voices[1];
    Reset();
}

SID::~SID() { Close(); }

void SID::Close() {
    if (audioStream != nullptr) {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
    }
}

void SID::Init(int sampleRate) {
    this->sampleRate = sampleRate;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "SDL Audio init failed: " << SDL_GetError() << '\n';
        return;
    }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = SDL_AUDIO_S16;
    want.channels = 1;

    audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, nullptr, nullptr);
    if (audioStream == nullptr) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << '\n';
    } else {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
    }
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
    }
    volumeRegister = 0;
    filterLow = 0.0;
    filterBand = 0.0;
    dcBlockerState = 0.0;
    dcBlockerPrevIn = 0.0;
    clockCounter = 0;
    fractionalCycles = 0.0;
    resampler.Clear();
    sampleBuffer.clear();
    
    if (audioStream != nullptr) {
        SDL_ClearAudioStream(audioStream);
    }
}

void SID::SetModel(SIDModel newModel) {
    std::lock_guard<std::mutex> lock(sidMutex);
    this->model = newModel;
}

SIDModel SID::GetModel() const {
    return model;
}

void SID::EnableSound(bool enable) {
    std::lock_guard<std::mutex> lock(sidMutex);
    
    if (!soundEnabled && enable) {
        // Clear old state before enabling
        resampler.Clear();
        dcBlockerState = 0.0;
        dcBlockerPrevIn = 0.0;
        if (audioStream != nullptr) {
            SDL_ClearAudioStream(audioStream);
        }
    }
    
    soundEnabled = enable;

#ifndef TARGET_WASM
    if (soundEnabled && !emulationPaused && !pendingFilename.empty()) {
        recorder = std::make_unique<AudioRecorder>();
        if (!recorder->Start(pendingFilename, sampleRate)) {
            recorder.reset();
        }
        pendingFilename.clear();
    }
#endif

    UpdateAudioState();
}

void SID::SetEmulationPaused(bool paused) {
    std::lock_guard<std::mutex> lock(sidMutex);
    
    if (emulationPaused && !paused) {
        // Resuming from pause: clear stale data to prevent audio pop
        resampler.Clear();
        dcBlockerState = 0.0;
        dcBlockerPrevIn = 0.0;
        if (audioStream != nullptr) {
            SDL_ClearAudioStream(audioStream);
        }
    }
    
    emulationPaused = paused;
#ifndef TARGET_WASM
    if (!emulationPaused && soundEnabled && !pendingFilename.empty()) {
        auto newRecorder = std::make_unique<AudioRecorder>();
        if (newRecorder->Start(pendingFilename, sampleRate)) {
            recorder = std::move(newRecorder);
            pendingFilename.clear();
        }
    }
#endif
    UpdateAudioState();
}

void SID::UpdateAudioState() {
    if (audioStream != nullptr) {
        SDL_AudioDeviceID devId = SDL_GetAudioStreamDevice(audioStream);
        if (!soundEnabled || emulationPaused) {
            SDL_PauseAudioDevice(devId);
        } else {
            SDL_ResumeAudioDevice(devId);
        }
    }
}

Byte SID::Read(Word addr) {
    uint8_t reg = addr & 0x1F;
    if (reg == 0x19 || reg == 0x1A) return 0xFF; // Paddle X/Y
    if (reg == 0x1B) return voice3OscOutput;
    if (reg == 0x1C) return voice3EnvOutput;
    return registers.at(reg);
}

void SID::Write(Word addr, Byte data) {
    uint8_t reg = addr & 0x1F;
    registers.at(reg) = data;
    
    if (reg == 0x18) {
        volumeRegister = data & 0x0F;
    }
    
    // Sync registers directly to voices for real-time accurate clocking
    for (int i = 0; i < 3; ++i) {
        int regOffset = i * 7;
        Oscillator& osc = voices.at(i);

        osc.frequency = registers.at(regOffset) | (registers.at(regOffset + 1) << 8);
        osc.pulseWidth = registers.at(regOffset + 2) | ((registers.at(regOffset + 3) & 0x0F) << 8);
        osc.control = registers.at(regOffset + 4);

        uint8_t attackDecay = registers.at(regOffset + 5);
        uint8_t sustainRelease = registers.at(regOffset + 6);
        osc.env.targetAttackRate = (attackDecay >> 4) & 0xF;
        osc.env.targetDecayRate = attackDecay & 0xF;
        osc.env.targetSustainLevel = (sustainRelease >> 4) & 0xF;
        osc.env.targetReleaseRate = sustainRelease & 0xF;
    }
    
    uint16_t fc = (registers.at(0x15) & 0x7) | (registers.at(0x16) << 3);
    uint8_t resFilt = registers.at(0x17);
    uint8_t modeVol = registers.at(0x18);
    filterFiltMask = resFilt & 0x0F;
    filterMode = modeVol & 0xF0;
}

void ADSREnvelope::Update(bool newGate) {
    bool gateRising = (!gate && newGate);
    bool gateFalling = (gate && !newGate);
    gate = newGate;

    if (gateRising) {
        state = ATTACK;
        attackRate = targetAttackRate;
        decayRate = targetDecayRate;
        sustainLevel = targetSustainLevel;
        releaseRate = targetReleaseRate;
        ratePeriod = RATE_COUNTER_PERIODS[attackRate];
        holdZero = false;
    } else if (gateFalling) {
        state = RELEASE;
        attackRate = targetAttackRate;
        decayRate = targetDecayRate;
        sustainLevel = targetSustainLevel;
        releaseRate = targetReleaseRate;
        ratePeriod = RATE_COUNTER_PERIODS[releaseRate];
    }

    // ADSR delay bug check
    if ((++rateCounter & 0x8000) != 0) {
        ++rateCounter &= 0x7FFF;
    }

    if (rateCounter != ratePeriod) {
        return;
    }

    rateCounter = 0;
    
    // Latch target registers on LFSR clock
    attackRate = targetAttackRate;
    decayRate = targetDecayRate;
    sustainLevel = targetSustainLevel;
    releaseRate = targetReleaseRate;
    
    if (state == ATTACK) ratePeriod = RATE_COUNTER_PERIODS[attackRate];
    else if (state == DECAY) ratePeriod = RATE_COUNTER_PERIODS[decayRate];
    else if (state == RELEASE) ratePeriod = RATE_COUNTER_PERIODS[releaseRate];

    if (state == ATTACK || ++exponentialCounter == exponentialCounterPeriod) {
        exponentialCounter = 0;

        if (holdZero) return;

        switch (state) {
            case ATTACK:
                ++level &= 0xFF;
                if (level == 0xFF) {
                    state = DECAY;
                    ratePeriod = RATE_COUNTER_PERIODS[decayRate];
                }
                break;
            case DECAY:
                if (level == SUSTAIN_LEVELS[sustainLevel]) {
                    state = SUSTAIN;
                    break;
                }
                --level;
                break;
            case SUSTAIN:
                if (level == SUSTAIN_LEVELS[sustainLevel]) {
                    break;
                }
                --level;
                break;
            case RELEASE:
                --level &= 0xFF;
                break;
            default:
                break;
        }

        switch (level) {
            case 0xFF: exponentialCounterPeriod = 1; break;
            case 0x5D: exponentialCounterPeriod = 2; break;
            case 0x36: exponentialCounterPeriod = 4; break;
            case 0x1A: exponentialCounterPeriod = 8; break;
            case 0x0E: exponentialCounterPeriod = 16; break;
            case 0x06: exponentialCounterPeriod = 30; break;
            case 0x00: 
                exponentialCounterPeriod = 1; 
                holdZero = true; 
                break;
        }
    }
}

void Oscillator::Next(SIDModel model) {
    if ((control & 0x08) != 0) {
        // Test bit resets and holds accumulator and noise
        accumulator = 0;
        noiseShift = 0x7FFFF8;
        oscOutput = 0;
        return;
    }

    uint32_t prevAcc = accumulator;
    accumulator = (accumulator + frequency) & 0xFFFFFF;

    // Hard Sync
    if ((control & 0x02) != 0 && prevOsc) {
        if (prevOsc->accumulator < prevOsc->frequency) {
            accumulator = 0;
        }
    }

    uint16_t out = 0;
    
    uint8_t waveControl = (control >> 4) & 0x0F;
    bool noiseOn = (waveControl & 0x08) != 0;
    bool pulseOn = (waveControl & 0x04) != 0;
    bool sawOn = (waveControl & 0x02) != 0;
    bool triOn = (waveControl & 0x01) != 0;
    
    // Update Noise Shift Register
    if (noiseOn) {
        if ((accumulator & 0x80000) != (prevAcc & 0x80000)) {
            uint32_t bit = ((noiseShift >> 22) ^ (noiseShift >> 17)) & 1;
            noiseShift = ((noiseShift << 1) & 0x7FFFFF) | bit;
        }
    }
    
    if (waveControl == 0) {
        out = 0;
    } else if (noiseOn) {
        uint16_t noise = (
            ((noiseShift & 0x400000) >> 11) |
            ((noiseShift & 0x100000) >> 10) |
            ((noiseShift & 0x010000) >> 7)  |
            ((noiseShift & 0x002000) >> 5)  |
            ((noiseShift & 0x000800) >> 4)  |
            ((noiseShift & 0x000080) >> 1)  |
            ((noiseShift & 0x000010) << 1)  |
            ((noiseShift & 0x000004) << 2)
        ) & 0xFFF;
        
        if (model == SIDModel::MOS6581 && waveControl != 0x08) {
            // Noise mixing bug: combining noise with other waveforms on 6581 sinks the output to 0V.
            out = 0;
        } else {
            out = noise;
        }
    } else {
        uint32_t ringMask = ((control & 0x04) != 0 && prevOsc) ? 0x800000 : 0;
        uint32_t syncAcc = prevOsc ? prevOsc->accumulator : 0;
        uint16_t ix = (accumulator ^ (~syncAcc & ringMask)) >> 12;
        
        uint16_t pulseOutput = ((accumulator >> 12) >= pulseWidth) ? 0xFFF : 0;
        if (control & 0x08) pulseOutput = 0xFFF; // Test bit sets pulse high
        
        if (model == SIDModel::MOS6581) {
            if (waveControl == 0x01) { // Tri
                uint32_t msb = accumulator & 0x800000;
                if (ringMask) msb ^= (syncAcc & 0x800000);
                out = ((accumulator ^ (msb ? 0xFFFFFF : 0)) >> 11) & 0xFFF;
            } else if (waveControl == 0x02) { // Saw
                out = ix;
            } else if (waveControl == 0x04) { // Pulse
                out = pulseOutput;
            } else if (waveControl == 0x03) { // Tri + Saw
                out = wave6581__ST[ix];
            } else if (waveControl == 0x05) { // Tri + Pulse
                out = wave6581_P_T[ix] & (pulseOutput | ~0xFFF);
            } else if (waveControl == 0x06) { // Saw + Pulse
                out = wave6581_PS_[ix] & (pulseOutput | ~0xFFF);
            } else if (waveControl == 0x07) { // Tri + Saw + Pulse
                out = wave6581_PST[ix] & (pulseOutput | ~0xFFF);
            }
        } else {
            // 8580 Clean logical AND mixing
            out = 0xFFF;
            if (triOn) {
                uint32_t msb = accumulator & 0x800000;
                if (ringMask) msb ^= (syncAcc & 0x800000);
                out &= ((accumulator ^ (msb ? 0xFFFFFF : 0)) >> 11) & 0xFFF;
            }
            if (sawOn) out &= ix;
            if (pulseOn) out &= pulseOutput;
        }
    }

    oscOutput = out;
}

void SID::Clock() {
    clockCounter++;
    
    for (int i = 0; i < 3; ++i) {
        voices[i].Next(model);
        voices[i].env.Update((voices[i].control & 0x01) != 0);
    }
    
    voice3OscOutput = (voices[2].oscOutput >> 4) & 0xFF;
    voice3EnvOutput = voices[2].env.level;
    
    if (!soundEnabled) return;
    
    // Process audio generation
    double filteredInput = 0.0;
    double directOutput = 0.0;
    
    uint16_t fc = (registers.at(0x15) & 0x7) | (registers.at(0x16) << 3);
    uint8_t resFilt = registers.at(0x17);
    uint8_t res = (resFilt >> 4) & 0x0F;
    
    double fcScaled = static_cast<double>(fc) / 2047.0;
    
    double cutoffHz;
    if (model == SIDModel::MOS6581) {
        // Approximated FET non-linear mapping (cubic curve)
        cutoffHz = 30.0 + 12000.0 * (fcScaled * fcScaled * fcScaled);
        
        // Q drop: on 6581, resonance dies out at high frequencies due to parasitic capacitance.
        double effectiveRes = (res / 15.0) * (1.0 - (fcScaled * fcScaled * 0.85));
        effectiveRes = std::max(0.0, effectiveRes);
        filterQ = 1.5 - (1.0 * effectiveRes);
    } else {
        cutoffHz = 30.0 + (fcScaled * 12000.0);
        filterQ = 2.0 - (1.8 * (res / 15.0));
    }
    
    // Clamp to prevent instability in filter coefficients
    cutoffHz = std::clamp(cutoffHz, 1.0, 24000.0);
    
    // ZDF Filter coefficients evaluated at SID_CLOCK (1,000,000 Hz)
    double w = 2.0 * 3.14159265358979323846 * cutoffHz / SID_CLOCK;
    double g = std::tan(w / 2.0);
    double R = 1.0 / filterQ;
    
    for (size_t v = 0; v < voices.size(); ++v) {
        double voiceSample = ((static_cast<double>(voices[v].oscOutput) / 2048.0) - 1.0) * (voices[v].env.level / 255.0);
        
        if ((filterFiltMask & (1 << v)) != 0) {
            filteredInput += voiceSample;
        } else {
            if (v == 2 && (filterMode & 0x80) != 0) continue;
            directOutput += voiceSample;
        }
    }
    
    // ZDF SVF equations
    double hp = (filteredInput - filterLow - filterBand * (R + g)) / (1.0 + g * (R + g));
    double bp = filterBand + g * hp;
    double lp = filterLow + g * bp;
    
    filterBand = bp + g * hp;
    filterLow = lp + g * bp;
    
    if (model == SIDModel::MOS6581) {
        // Non-linear OTA clipping
        filterBand = std::tanh(filterBand);
        filterLow = std::tanh(filterLow);
    }
    
    double filterOutput = 0.0;
    if ((filterMode & 0x10) != 0) filterOutput += filterLow;
    if ((filterMode & 0x20) != 0) filterOutput += filterBand;
    if ((filterMode & 0x40) != 0) filterOutput += hp;
    
    if (model == SIDModel::MOS6581) {
        filterOutput += filteredInput * 0.05; // Bass leakage
    }
    
    double mix = (directOutput + filterOutput) * 0.25;
    mix = std::tanh(mix);
    
    uint8_t currentVolume = volumeRegister & 0x0F;
    mix *= (currentVolume / 15.0);
    
    if (model == SIDModel::MOS6581) {
        // Volume DAC Bug: The 6581 master volume control has a severe DC offset bug.
        // This causes clicks when the volume changes, heavily exploited for PCM playback.
        double volumeOffset = (currentVolume / 15.0) - 0.5;
        mix += volumeOffset * 0.45; // Depth of the PCM volume bug
    }
    
    // The DC Blocker must be very slow for 6581 to allow PCM samples (which are effectively low freq jumps)
    // to pass through, but fast enough to prevent long-term DC drift.
    double pole = (model == SIDModel::MOS6581) ? 0.99995 : 0.995;
    double filteredMix = mix - dcBlockerPrevIn + (pole * dcBlockerState);
    dcBlockerState = filteredMix;
    dcBlockerPrevIn = mix;
    mix = filteredMix;
    
    resampler.PushSample(mix);
    
    while (resampler.HasOutput()) {
        double finalMix = resampler.GetOutput();
        sampleBuffer.push_back(static_cast<int16_t>(finalMix * 18000.0));
        
        if (sampleBuffer.size() >= 512) {
            if (audioStream != nullptr && !emulationPaused) {
                SDL_PutAudioStreamData(audioStream, sampleBuffer.data(), sampleBuffer.size() * sizeof(int16_t));
            }
#ifndef TARGET_WASM
            if (recorder && soundEnabled && !emulationPaused) {
                recorder->PushAudio(sampleBuffer.data(), sampleBuffer.size());
            }
#endif
            if (audioCallback) {
                audioCallback(sampleBuffer.data(), sampleBuffer.size());
            }
            sampleBuffer.clear();
        }
    }
}

// Keep stubs for compatibility
void SID::AudioCallback(void*, SDL_AudioStream*, int, int) {}
void SID::GenerateAudio(int16_t*, int) {}

bool ADSREnvelope::SaveState(std::ostream& out) const {
    Hardware::ISerializable::Serialize(out, state);
    Hardware::ISerializable::Serialize(out, level);
    Hardware::ISerializable::Serialize(out, rateCounter);
    Hardware::ISerializable::Serialize(out, attackRate);
    Hardware::ISerializable::Serialize(out, decayRate);
    Hardware::ISerializable::Serialize(out, sustainLevel);
    Hardware::ISerializable::Serialize(out, releaseRate);
    return out.good();
}

bool ADSREnvelope::LoadState(std::istream& inStream) {
    Hardware::ISerializable::Deserialize(inStream, state);
    Hardware::ISerializable::Deserialize(inStream, level);
    Hardware::ISerializable::Deserialize(inStream, rateCounter);
    Hardware::ISerializable::Deserialize(inStream, attackRate);
    Hardware::ISerializable::Deserialize(inStream, decayRate);
    Hardware::ISerializable::Deserialize(inStream, sustainLevel);
    Hardware::ISerializable::Deserialize(inStream, releaseRate);
    return inStream.good();
}

bool Oscillator::SaveState(std::ostream& out) const {
    Hardware::ISerializable::Serialize(out, accumulator);
    Hardware::ISerializable::Serialize(out, frequency);
    Hardware::ISerializable::Serialize(out, pulseWidth);
    Hardware::ISerializable::Serialize(out, control);
    Hardware::ISerializable::Serialize(out, noiseShift);
    Hardware::ISerializable::Serialize(out, oscOutput);
    env.SaveState(out);
    return out.good();
}

bool Oscillator::LoadState(std::istream& inStream) {
    Hardware::ISerializable::Deserialize(inStream, accumulator);
    Hardware::ISerializable::Deserialize(inStream, frequency);
    Hardware::ISerializable::Deserialize(inStream, pulseWidth);
    Hardware::ISerializable::Deserialize(inStream, control);
    Hardware::ISerializable::Deserialize(inStream, noiseShift);
    Hardware::ISerializable::Deserialize(inStream, oscOutput);
    env.LoadState(inStream);
    return inStream.good();
}

bool SID::SaveState(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(sidMutex);
    ISerializable::Serialize(out, registers);
    for (const auto& voice : voices) {
        voice.SaveState(out);
    }
    ISerializable::Serialize(out, volumeRegister);
    ISerializable::Serialize(out, soundEnabled);
    ISerializable::Serialize(out, emulationPaused);
    ISerializable::Serialize(out, filterLow);
    ISerializable::Serialize(out, filterBand);
    ISerializable::Serialize(out, dcBlockerState);
    ISerializable::Serialize(out, dcBlockerPrevIn);
    
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
    ISerializable::Deserialize(inStream, filterLow);
    ISerializable::Deserialize(inStream, filterBand);
    ISerializable::Deserialize(inStream, dcBlockerState);
    ISerializable::Deserialize(inStream, dcBlockerPrevIn);
    
    uint8_t mod = 0;
    if (!inStream.eof()) {
        ISerializable::Deserialize(inStream, mod);
        model = static_cast<SIDModel>(mod);
    }
    UpdateAudioState();
    return inStream.good();
}

void SID::StartRecording(const std::string& filename) {
    std::lock_guard<std::mutex> lock(sidMutex);
#ifndef TARGET_WASM
    if (!emulationPaused && soundEnabled) {
        recorder = std::make_unique<AudioRecorder>();
        if (!recorder->Start(filename, sampleRate)) {
            recorder.reset();
        }
    } else {
        pendingFilename = filename;
    }
#else
    (void)filename;
#endif
}

void SID::StopRecording() {
    std::lock_guard<std::mutex> lock(sidMutex);
    pendingFilename.clear();
#ifndef TARGET_WASM
    if (recorder) {
        recorder->Stop();
        recorder.reset();
    }
#endif
}

bool SID::IsRecording() const {
    std::lock_guard<std::mutex> lock(sidMutex);
#ifndef TARGET_WASM
    return recorder != nullptr || !pendingFilename.empty();
#else
    return !pendingFilename.empty();
#endif
}

void SID::SetAudioCallback(std::function<void(const int16_t*, int)> callback) {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioCallback = std::move(callback);
}

void SID::ClearAudioCallback() {
    std::lock_guard<std::mutex> lock(sidMutex);
    audioCallback = nullptr;
}

}  // namespace Hardware
