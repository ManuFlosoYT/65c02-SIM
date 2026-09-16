#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>

#ifndef TARGET_WASM
#include "Hardware/Audio/AudioRecorder.h"
#endif
#include "Hardware/Core/IBusDevice.h"
#include <memory>
#include <optional>

namespace Hardware {

enum class SIDModel { MOS6581, MOS8580 };

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
    int attackRate = 0;
    int decayRate = 0;
    int sustainLevel = 0;
    int releaseRate = 0;

    void Update(bool gate);
    double Next();

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

    void Next(SIDModel model);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);
};

class SID : public IBusDevice {
   public:
    static constexpr int MAX_SID_VOICES = 3;
    
    SID();
    ~SID() override;
    SID(const SID&) = delete;
    SID& operator=(const SID&) = delete;
    SID(SID&&) = delete;
    SID& operator=(SID&&) = delete;

    void Init(int sampleRate = 48000);
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
    void Clock();

    void StartRecording(const std::string& filename);
    void StopRecording();
    bool IsRecording() const;
    void SetAudioCallback(std::function<void(const int16_t*, int)> callback);
    void ClearAudioCallback();

   private:
    std::array<std::uint8_t, 0x20> registers{};
    std::array<Oscillator, MAX_SID_VOICES> voices{};
    std::uint8_t volumeRegister{0};
    bool soundEnabled = false;
    bool emulationPaused = true;
    
    SIDModel model = SIDModel::MOS8580;
    
    // Cycle accuracy
    uint64_t clockCounter = 0;
    double fractionalCycles = 0.0;
    
    // Buffer for audio generated during Clock()
    std::vector<int16_t> sampleBuffer;
    
    // Cache for read-only registers
    uint8_t voice3OscOutput = 0;
    uint8_t voice3EnvOutput = 0;

    double filterLow = 0.0;
    double filterBand = 0.0;
    double filterF = 0.0;
    double filterQ = 1.0;
    std::uint8_t filterFiltMask = 0;
    std::uint8_t filterMode = 0;

    double dcBlockerState = 0.0;
    double dcBlockerPrevIn = 0.0;

    SDL_AudioStream* audioStream = nullptr;
    int sampleRate = 48000;
    mutable std::mutex sidMutex;

#ifndef TARGET_WASM
    std::unique_ptr<AudioRecorder> recorder;
#endif
    std::string pendingFilename;
    std::function<void(const int16_t*, int)> audioCallback;

    static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                               int total_amount);
    void GenerateAudio(int16_t* buffer, int length);
    void UpdateAudioState();
};

}  // namespace Hardware

inline std::string Hardware::SID::GetName() const { return "SID"; }
inline bool Hardware::SID::IsSoundEnabled() const { return soundEnabled; }
inline const Hardware::Oscillator& Hardware::SID::GetVoice(int index) const {
    return voices.at(index % MAX_SID_VOICES);
}
