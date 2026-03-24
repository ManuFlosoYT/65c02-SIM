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

namespace Hardware {

struct ADSREnvelope {
    enum State : std::uint8_t { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = IDLE;
    double level = 0.0;

    // Parameters from registers
    int attackRate = 0;
    int decayRate = 0;
    double sustainLevel = 0.0;
    int releaseRate = 0;

    void Update(bool gate, int sampleRate = 48000);
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

    // Noise generation
    uint32_t noiseShift = 0x7FFFF8;

    double Next(int sampleRate = 48000);

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
    void Write(Word addr, Byte data) override;  // NOLINT(bugprone-easily-swappable-parameters)
    std::string GetName() const override;

    void EnableSound(bool enable);
    void SetEmulationPaused(bool paused);

    bool IsSoundEnabled() const;

    const Oscillator& GetVoice(int index) const;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

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

    // Audio Stream
    SDL_AudioStream* audioStream = nullptr;
    int sampleRate = 48000;
    mutable std::mutex sidMutex;

#ifndef TARGET_WASM
    std::unique_ptr<AudioRecorder> recorder;
#endif
    std::string pendingFilename;
    std::function<void(const int16_t*, int)> audioCallback;

    static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                              int total_amount);  // NOLINT(bugprone-easily-swappable-parameters)
    void GenerateAudio(int16_t* buffer, int length);
    void UpdateAudioState();
};

}  // namespace Hardware

inline std::string Hardware::SID::GetName() const { return "SID"; }
inline bool Hardware::SID::IsSoundEnabled() const { return soundEnabled; }
inline const Hardware::Oscillator& Hardware::SID::GetVoice(int index) const {
    return voices.at(index % MAX_SID_VOICES);
}
