#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <span>
#include <vector>

#include "Hardware/Core/IBusDevice.h"
#include "Hardware/Audio/SIDFilter.h"
#include "Hardware/Audio/SIDAudioSink.h"
#include "Hardware/Audio/SIDTrackerLogger.h"
#include <memory>

namespace Hardware {

struct ADSREnvelope {
    enum State : std::uint8_t { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = IDLE;
    uint8_t level = 0;
    
    // 15-bit LFSR / Rate Counter and exponential divider logic
    uint16_t rateCounter = 0;
    uint16_t ratePeriod = 0;
    uint8_t exponentialCounter = 0;
    uint8_t exponentialCounterPeriod = 1;
    bool holdZero = true;
    bool gate = false;

    // Parameters from registers
    uint8_t attackRate = 0;
    uint8_t decayRate = 0;
    uint8_t sustainLevel = 0;
    uint8_t releaseRate = 0;

    // Registers updated by CPU, applied to the active rates only when the LFSR ticks
    uint8_t targetAttackRate = 0;
    uint8_t targetDecayRate = 0;
    uint8_t targetSustainLevel = 0;
    uint8_t targetReleaseRate = 0;

    void Update(bool gate);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);
};

struct Oscillator {
    uint32_t accumulator = 0;
    uint32_t frequency = 0;
    uint16_t pulseWidth = 0;
    uint8_t control = 0;  // Gate, Sync, Ring, Test, Tri, Saw, Pulse, Noise

    ADSREnvelope env;
    Oscillator* prevOsc = nullptr; // For Hard Sync and Ring Mod

    // Noise generation
    uint32_t noiseShift = 0x7FFFF8;
    
    // Output state
    uint16_t oscOutput = 0;

    uint64_t lastGateOffClock = 0;
    bool gateWasActive = false;
    bool hadHardRestart = false;
    std::atomic<uint64_t> adsrDelayBugClock{0};

    [[nodiscard]] uint64_t GetADSRDelayBugClock() const { return adsrDelayBugClock.load(std::memory_order_relaxed); }

    void Next(SIDModel model);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);
};

struct SIDProfilingStats {
    std::uint64_t clockCount = 0;
    std::uint64_t oscillatorNanoseconds = 0;
    std::uint64_t envelopeNanoseconds = 0;
    std::uint64_t filterNanoseconds = 0;
    std::uint64_t resamplerNanoseconds = 0;
    std::uint64_t outputNanoseconds = 0;
};

class SID : public IBusDevice {
   public:
    static constexpr int MAX_SID_VOICES = 3;
    static constexpr double SID_NATIVE_RATE = 1000000.0;
    
    SID();
    ~SID() override;
    SID(const SID&) = delete;
    SID& operator=(const SID&) = delete;
    SID(SID&&) = delete;
    SID& operator=(SID&&) = delete;

    void Init(int sampleRate = 48000);
    void SetClockRate(double clockRate);
    void SetCpuClockRate(double rate);
    void Close();
    void Reset() override;
    Byte Read(Word addr) override;
    void Write(Word addr, Byte data) override;
    std::string GetName() const override;

    void EnableSound(bool enable);
    void SetEmulationPaused(bool paused);
    void SetModel(SIDModel model);
    SIDModel GetModel() const;

    bool IsSoundEnabled() const;

    const Oscillator& GetVoice(int index) const;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;
    void Clock() {
        if (isDormant) [[likely]] {
            return;
        }
        sidCycleAccumulator += static_cast<uint64_t>(SID_NATIVE_RATE);
        while (sidCycleAccumulator >= cpuClockRateInt) {
            sidCycleAccumulator -= cpuClockRateInt;
            ClockSlow();
        }
    }

    void Clock(uint32_t cycles) {
        if (isDormant) [[likely]] {
            return;
        }
        sidCycleAccumulator += static_cast<uint64_t>(SID_NATIVE_RATE) * cycles;
        while (sidCycleAccumulator >= cpuClockRateInt) {
            sidCycleAccumulator -= cpuClockRateInt;
            ClockSlow();
        }
    }

    [[nodiscard]] bool IsDormant() const { return isDormant; }

    void StartRecording(const std::string& filename);
    void StopRecording();
    bool IsRecording() const;
    void SetAudioCallback(std::function<void(const int16_t*, int)> callback);
    void ClearAudioCallback();
    void SetProfilingEnabled(bool enabled);
    void ClearProfilingStats();
    SIDProfilingStats GetProfilingStats() const;

    void SetVoiceMute(int index, bool mute);
    [[nodiscard]] bool IsVoiceMuted(int index) const;
    void SetVoiceSolo(int index, bool solo);
    [[nodiscard]] bool IsVoiceSolo(int index) const;
    void GetAudioSnapshot(std::span<float> outBuffer) const;
    [[nodiscard]] const std::array<std::uint8_t, 0x20>& GetRegisters() const { return registers; }
    [[nodiscard]] uint8_t GetRegister(size_t reg) const { return registers.at(reg & 0x1F); }
    void GetTrackerLog(std::vector<SIDTrackerFrame>& outLog) const;
    void ClearTrackerLog();

   private:
    std::array<std::uint8_t, 0x20> registers{};
    std::array<Oscillator, MAX_SID_VOICES> voices{};
    std::uint8_t volumeRegister{0};
    bool soundEnabled = false;
    bool emulationPaused = true;
    
    SIDModel model = SIDModel::MOS8580;
    
    // Cycle accuracy
    uint64_t clockCounter = 0;
    uint64_t dormantClockAnchor = 0;
    bool isDormant = true;
    double clockRate = SID_NATIVE_RATE;
    uint64_t sidCycleAccumulator = 0;
    uint64_t cpuClockRateInt = 1000000;
    
    // Cache for read-only registers
    uint8_t voice3OscOutput = 0;
    uint8_t voice3EnvOutput = 0;

    std::uint8_t filterFiltMask = 0;
    std::uint8_t filterMode = 0;
    SIDFilter filter;

    SIDResampler resampler;
    SIDAudioSink audioSink;
    SIDTrackerLogger trackerLogger;

    mutable std::mutex sidMutex;
    bool profilingEnabled = false;
    SIDProfilingStats profilingStats;

    std::atomic<uint8_t> voiceMuteMask{0};
    std::atomic<uint8_t> voiceSoloMask{0};

    template <bool Profiled>
    void ClockInternal();
    void UpdateAudioState();
    void ClockSlow();
    void UpdateDormantState();
    void SyncDormantEnvelopes();
    void ClockProfiled();
    void UpdateFilterCoefficients();
    void FlushAvailableOutput();
};

}  // namespace Hardware

inline std::string Hardware::SID::GetName() const { return "SID"; }
inline bool Hardware::SID::IsSoundEnabled() const { return soundEnabled; }
inline const Hardware::Oscillator& Hardware::SID::GetVoice(int index) const {
    return voices.at(index % MAX_SID_VOICES);
}

inline void Hardware::SID::SetVoiceMute(int index, bool mute) {
    if (index < 0 || index >= MAX_SID_VOICES) {
        return;
    }
    if (mute) {
        voiceMuteMask.fetch_or(static_cast<uint8_t>(1U << index), std::memory_order_relaxed);
    } else {
        voiceMuteMask.fetch_and(static_cast<uint8_t>(~(1U << index)), std::memory_order_relaxed);
    }
}

inline bool Hardware::SID::IsVoiceMuted(int index) const {
    if (index < 0 || index >= MAX_SID_VOICES) {
        return false;
    }
    return (voiceMuteMask.load(std::memory_order_relaxed) & (1U << index)) != 0;
}

inline void Hardware::SID::SetVoiceSolo(int index, bool solo) {
    if (index < 0 || index >= MAX_SID_VOICES) {
        return;
    }
    if (solo) {
        voiceSoloMask.fetch_or(static_cast<uint8_t>(1U << index), std::memory_order_relaxed);
    } else {
        voiceSoloMask.fetch_and(static_cast<uint8_t>(~(1U << index)), std::memory_order_relaxed);
    }
}

inline bool Hardware::SID::IsVoiceSolo(int index) const {
    if (index < 0 || index >= MAX_SID_VOICES) {
        return false;
    }
    return (voiceSoloMask.load(std::memory_order_relaxed) & (1U << index)) != 0;
}
