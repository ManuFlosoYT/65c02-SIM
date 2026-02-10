#include "SID.h"

#include <SDL.h>

#include <cstring>
#include <iostream>

// SID clock frequency ~1.0 MHz
constexpr double SID_CLOCK = 985248.0;

// Simple LCG for fast noise
static uint32_t fast_rand() {
    static uint32_t x = 123456789;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// Lookup tables for ADSR roughly based on SID specs
const double ATTACK_RATES[16] = {0.002, 0.008, 0.016, 0.024, 0.038, 0.056,
                                 0.068, 0.080, 0.100, 0.250, 0.500, 0.800,
                                 1.000, 3.000, 5.000, 8.000};
const double DECAY_RELEASE_RATES[16] = {
    0.006, 0.024, 0.048, 0.072, 0.114, 0.168, 0.204,  0.240,
    0.300, 0.750, 1.500, 2.400, 3.000, 9.000, 15.000, 24.000};

SID::SID() { Reset(); }

SID::~SID() {
    if (devId != 0) {
        SDL_CloseAudioDevice(devId);
    }
}

void SID::Init(int sampleRate) {
    this->sampleRate = sampleRate;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Audio init failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = AudioCallback;
    want.userdata = this;

    devId = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (devId == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    } else {
        /* std::cout << "SID Initialized. Audio Device ID: " << devId
                  << " Sample Rate: " << have.freq << std::endl; */
        this->sampleRate = have.freq;
    }
}

void SID::Reset() {
    std::lock_guard<std::mutex> lock(sidMutex);
    std::memset(registers, 0, sizeof(registers));
    for (int i = 0; i < 3; ++i) {
        voices[i].accumulator = 0;
        voices[i].frequency = 0;
        voices[i].pulseWidth = 0;
        voices[i].control = 0;
        voices[i].noiseShift = 0x7FFFF8;
        voices[i].env.state = ADSREnvelope::IDLE;
        voices[i].env.level = 0;
    }
    volumeRegister = 0;
}

void SID::EnableSound(bool enable) {
    std::lock_guard<std::mutex> lock(sidMutex);
    soundEnabled = enable;
    UpdateAudioState();
}

void SID::SetEmulationPaused(bool paused) {
    std::lock_guard<std::mutex> lock(sidMutex);
    emulationPaused = paused;
    UpdateAudioState();
}

void SID::UpdateAudioState() {
    if (devId != 0) {
        // Paused if sound disabled OR emulation paused
        // SDL_PauseAudioDevice: 1 = pause, 0 = unpause
        int pause = (!soundEnabled || emulationPaused) ? 1 : 0;
        SDL_PauseAudioDevice(devId, pause);
    }
}

uint8_t SID::Read(uint16_t addr) {
    std::lock_guard<std::mutex> lock(sidMutex);
    return registers[addr & 0x1F];
}

void SID::Write(uint16_t addr, uint8_t data) {
    std::lock_guard<std::mutex> lock(sidMutex);
    uint8_t reg = addr & 0x1F;
    registers[reg] = data;
    if (reg == 0x18) {
        volumeRegister = data & 0x0F;
    }
}

void SID::AudioCallback(void* userdata, uint8_t* stream, int len) {
    SID* sid = static_cast<SID*>(userdata);
    sid->GenerateAudio(reinterpret_cast<int16_t*>(stream), len / 2);
}

void SID::GenerateAudio(int16_t* buffer, int length) {
    uint8_t currentVolume = 0;

    // 1. Synchronization Phase: Copy parameters from registers to voices (under
    // lock)
    {
        std::lock_guard<std::mutex> lock(sidMutex);

        if (!soundEnabled) {
            std::memset(buffer, 0, length * sizeof(int16_t));
            return;
        }

        currentVolume = volumeRegister & 0x0F;

        for (int i = 0; i < 3; ++i) {
            int regOffset = i * 7;
            Oscillator& v = voices[i];

            // Frequency
            v.frequency =
                registers[regOffset] | (registers[regOffset + 1] << 8);

            // Pulse Width
            v.pulseWidth = registers[regOffset + 2] |
                           ((registers[regOffset + 3] & 0x0F) << 8);

            // Control
            v.control = registers[regOffset + 4];

            // ADSR
            uint8_t ad = registers[regOffset + 5];
            uint8_t sr = registers[regOffset + 6];
            v.env.attackRate = (ad >> 4) & 0xF;
            v.env.decayRate = ad & 0xF;
            v.env.sustainLevel = ((sr >> 4) & 0xF) / 15.0;
            v.env.releaseRate = sr & 0xF;

            // Handle Test Bit (Reset Accumulator)
            if (v.control & 0x08) {
                v.accumulator = 0;
            }
        }
    }  // Unlock mutex immediately

    // 2. Generation Phase (No Lock)
    for (int i = 0; i < length; ++i) {
        double mix = 0.0;
        for (int v = 0; v < 3; ++v) {
            bool gate = (voices[v].control & 0x01);
            voices[v].env.Update(gate, sampleRate);
            mix += voices[v].Next(sampleRate);
        }

        double vol = currentVolume / 15.0;
        mix *= vol;

        if (mix > 1.0) mix = 1.0;
        if (mix < -1.0) mix = -1.0;

        buffer[i] = static_cast<int16_t>(mix * 5000.0);
    }
}

// ADSREnvelope Implementation
void ADSREnvelope::Update(bool gate, int sampleRate) {
    if (gate) {
        if (state == IDLE || state == RELEASE) {
            state = ATTACK;
        }
    } else {
        if (state != IDLE && state != RELEASE) {
            state = RELEASE;
        }
    }

    double rate = 0;
    switch (state) {
        case ATTACK:
            rate = 1.0 / (ATTACK_RATES[attackRate] * sampleRate);
            level += rate;
            if (level >= 1.0) {
                level = 1.0;
                state = DECAY;
            }
            break;
        case DECAY:
            rate = 1.0 / (DECAY_RELEASE_RATES[decayRate] * sampleRate);
            level -= rate;
            if (level <= sustainLevel) {
                level = sustainLevel;
                state = SUSTAIN;
            }
            break;
        case SUSTAIN:
            if (level > sustainLevel) {
                rate = 1.0 / (DECAY_RELEASE_RATES[decayRate] * sampleRate);
                level -= rate;
            } else {
                level = sustainLevel;
            }
            break;
        case RELEASE:
            rate = 1.0 / (DECAY_RELEASE_RATES[releaseRate] * sampleRate);
            level -= rate;
            if (level <= 0) {
                level = 0;
                state = IDLE;
            }
            break;
        case IDLE:
            level = 0;
            break;
    }
}

// Oscillator Implementation
double Oscillator::Next(int sampleRate) {
    if (frequency == 0) return 0.0;

    double step24 = frequency * (SID_CLOCK / sampleRate);
    accumulator = (accumulator + static_cast<uint32_t>(step24)) & 0xFFFFFF;

    double output = 0.0;

    if (control & 0x80) {  // Noise
        output = static_cast<double>(fast_rand()) / 0xFFFFFFFF * 2.0 - 1.0;
    } else if (control & 0x10) {  // Triangle
        uint32_t temp = accumulator;
        if (temp & 0x800000) temp ^= 0xFFFFFF;
        output = (static_cast<double>(temp) / 0x400000) - 1.0;
    } else if (control & 0x20) {  // Sawtooth
        output = (static_cast<double>(accumulator) / 0x800000) - 1.0;
    } else if (control & 0x40) {  // Pulse
        uint16_t accHigh = (accumulator >> 12) & 0xFFF;
        if (accHigh >= (pulseWidth & 0xFFF)) {
            output = 1.0;
        } else {
            output = -1.0;
        }
    }

    return output * env.level;
}
