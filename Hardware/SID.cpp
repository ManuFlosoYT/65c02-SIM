#include "SID.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <iostream>
#include <vector>

namespace Hardware {

constexpr double SID_CLOCK = 1000000.0;

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
        std::cerr << "SDL Audio init failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = SDL_AUDIO_S16;
    want.channels = 1;

    audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &want, AudioCallback, this);
    if (audioStream == nullptr) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    } else {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
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
        voices[i].env.attackRate = 0;
        voices[i].env.decayRate = 0;
        voices[i].env.sustainLevel = 0.0;
        voices[i].env.releaseRate = 0;
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
    if (audioStream != nullptr) {
        SDL_AudioDeviceID devId = SDL_GetAudioStreamDevice(audioStream);
        if (!soundEnabled || emulationPaused) {
            SDL_PauseAudioDevice(devId);
        } else {
            SDL_ResumeAudioDevice(devId);
        }
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

void SID::AudioCallback(void* userdata, SDL_AudioStream* stream,
                        int additional_amount, int total_amount) {
    if (additional_amount > 0) {
        SID* sid = static_cast<SID*>(userdata);
        int samples = additional_amount / sizeof(int16_t);
        std::vector<int16_t> buffer(samples);
        sid->GenerateAudio(buffer.data(), samples);
        SDL_PutAudioStreamData(stream, buffer.data(), additional_amount);
    }
}

void SID::GenerateAudio(int16_t* buffer, int length) {
    uint8_t currentVolume = 0;

    // Synchronization Phase: Copy parameters from registers to voices (under
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
    }  // Unlock mutex

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

bool ADSREnvelope::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&state), sizeof(state));
    out.write(reinterpret_cast<const char*>(&level), sizeof(level));
    out.write(reinterpret_cast<const char*>(&attackRate), sizeof(attackRate));
    out.write(reinterpret_cast<const char*>(&decayRate), sizeof(decayRate));
    out.write(reinterpret_cast<const char*>(&sustainLevel),
              sizeof(sustainLevel));
    out.write(reinterpret_cast<const char*>(&releaseRate), sizeof(releaseRate));
    return out.good();
}

bool ADSREnvelope::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&state), sizeof(state));
    in.read(reinterpret_cast<char*>(&level), sizeof(level));
    in.read(reinterpret_cast<char*>(&attackRate), sizeof(attackRate));
    in.read(reinterpret_cast<char*>(&decayRate), sizeof(decayRate));
    in.read(reinterpret_cast<char*>(&sustainLevel), sizeof(sustainLevel));
    in.read(reinterpret_cast<char*>(&releaseRate), sizeof(releaseRate));
    return in.good();
}

bool Oscillator::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&accumulator), sizeof(accumulator));
    out.write(reinterpret_cast<const char*>(&frequency), sizeof(frequency));
    out.write(reinterpret_cast<const char*>(&pulseWidth), sizeof(pulseWidth));
    out.write(reinterpret_cast<const char*>(&control), sizeof(control));
    out.write(reinterpret_cast<const char*>(&noiseShift), sizeof(noiseShift));
    env.SaveState(out);
    return out.good();
}

bool Oscillator::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&accumulator), sizeof(accumulator));
    in.read(reinterpret_cast<char*>(&frequency), sizeof(frequency));
    in.read(reinterpret_cast<char*>(&pulseWidth), sizeof(pulseWidth));
    in.read(reinterpret_cast<char*>(&control), sizeof(control));
    in.read(reinterpret_cast<char*>(&noiseShift), sizeof(noiseShift));
    env.LoadState(in);
    return in.good();
}

bool SID::SaveState(std::ostream& out) {
    std::lock_guard<std::mutex> lock(sidMutex);
    out.write(reinterpret_cast<const char*>(registers), sizeof(registers));
    for (int i = 0; i < MAX_SID_VOICES; ++i) {
        voices[i].SaveState(out);
    }
    out.write(reinterpret_cast<const char*>(&volumeRegister),
              sizeof(volumeRegister));
    out.write(reinterpret_cast<const char*>(&soundEnabled),
              sizeof(soundEnabled));
    out.write(reinterpret_cast<const char*>(&emulationPaused),
              sizeof(emulationPaused));
    return out.good();
}

bool SID::LoadState(std::istream& in) {
    std::lock_guard<std::mutex> lock(sidMutex);
    in.read(reinterpret_cast<char*>(registers), sizeof(registers));
    for (int i = 0; i < MAX_SID_VOICES; ++i) {
        voices[i].LoadState(in);
    }
    in.read(reinterpret_cast<char*>(&volumeRegister), sizeof(volumeRegister));
    in.read(reinterpret_cast<char*>(&soundEnabled), sizeof(soundEnabled));
    in.read(reinterpret_cast<char*>(&emulationPaused), sizeof(emulationPaused));
    UpdateAudioState();
    return in.good();
}

}  // namespace Hardware
