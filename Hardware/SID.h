#ifndef SIM_65C02_SID_H
#define SIM_65C02_SID_H

#include <SDL.h>

#include <cstdint>
#include <mutex>

struct ADSREnvelope {
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = IDLE;
    double level = 0.0;

    // Parameters from registers
    int attackRate;
    int decayRate;
    double sustainLevel;
    int releaseRate;

    void Update(bool gate, int sampleRate = 44100);
    double Next();
};

struct Oscillator {
    uint32_t accumulator = 0;
    uint32_t frequency = 0;
    uint16_t pulseWidth = 0;
    uint8_t control = 0;  // Gate, Sync, Ring, Test, Tri, Saw, Pulse, Noise

    ADSREnvelope env;

    // Noise generation
    uint32_t noiseShift = 0x7FFFF8;

    double Next(int sampleRate = 44100);
};

class SID {
public:
    #define MAX_SID_VOICES 3
    SID();
    ~SID();

    void Init(int sampleRate = 44100);
    void Reset();
    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    void EnableSound(bool enable);
    void SetEmulationPaused(bool paused);
    bool IsSoundEnabled() const { return soundEnabled; }
    const Oscillator& GetVoice(int index) const { return voices[index % MAX_SID_VOICES]; }

private:
    uint8_t registers[0x20];
    Oscillator voices[MAX_SID_VOICES];
    uint8_t volumeRegister;
    bool soundEnabled = false;
    bool emulationPaused = false;

    // Audio device ID
    SDL_AudioDeviceID devId = 0;
    int sampleRate = 44100;
    mutable std::mutex sidMutex;

    static void AudioCallback(void* userdata, uint8_t* stream, int len);
    void GenerateAudio(int16_t* buffer, int length);
    void UpdateAudioState();
};

#endif  // SIM_65C02_SID_H
