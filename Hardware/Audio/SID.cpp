#include "Hardware/Audio/SID.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace Hardware {

constexpr double SID_CLOCK = 1000000.0;

// Simple LCG for fast noise
static uint32_t fast_rand() {
    static uint32_t seed = 123456789;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// Lookup tables for ADSR roughly based on SID specs
constexpr std::array<double, 16> ATTACK_RATES = {0.002, 0.008, 0.016, 0.024, 0.038, 0.056, 0.068, 0.080,
                                                 0.100, 0.250, 0.500, 0.800, 1.000, 3.000, 5.000, 8.000};

constexpr std::array<double, 16> DECAY_RELEASE_RATES = {0.006, 0.024, 0.048, 0.072, 0.114, 0.168, 0.204,  0.240,
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
        std::cerr << "SDL Audio init failed: " << SDL_GetError() << '\n';
        return;
    }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = SDL_AUDIO_S16;
    want.channels = 1;

    audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, AudioCallback, this);
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
        voice.env.state = ADSREnvelope::IDLE;
        voice.env.level = 0;
        voice.env.attackRate = 0;
        voice.env.decayRate = 0;
        voice.env.sustainLevel = 0.0;
        voice.env.releaseRate = 0;
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

Byte SID::Read(Word addr) {
    std::lock_guard<std::mutex> lock(sidMutex);
    return registers.at(addr & 0x1F);
}

void SID::Write(Word addr, Byte data) {  // NOLINT(bugprone-easily-swappable-parameters)
    std::lock_guard<std::mutex> lock(sidMutex);
    uint8_t reg = addr & 0x1F;
    registers.at(reg) = data;
    if (reg == 0x18) {
        volumeRegister = data & 0x0F;
    }
}

void SID::AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                        int total_amount) {  // NOLINT(bugprone-easily-swappable-parameters)
    if (additional_amount > 0) {
        SID* sid = static_cast<SID*>(userdata);
        int samples = static_cast<int>(additional_amount / sizeof(int16_t));
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
            Oscillator& osc = voices.at(i);

            // Frequency
            osc.frequency = registers.at(regOffset) | (registers.at(regOffset + 1) << 8);

            // Pulse Width
            osc.pulseWidth = registers.at(regOffset + 2) | ((registers.at(regOffset + 3) & 0x0F) << 8);

            // Control
            osc.control = registers.at(regOffset + 4);

            // ADSR
            uint8_t attackDecay = registers.at(regOffset + 5);
            uint8_t sustainRelease = registers.at(regOffset + 6);
            osc.env.attackRate = (attackDecay >> 4) & 0xF;
            osc.env.decayRate = attackDecay & 0xF;
            osc.env.sustainLevel = ((sustainRelease >> 4) & 0xF) / 15.0;
            osc.env.releaseRate = sustainRelease & 0xF;

            // Handle Test Bit (Reset Accumulator)
            if ((osc.control & 0x08) != 0) {
                osc.accumulator = 0;
            }
        }
    }  // Unlock mutex

    // 2. Generation Phase (No Lock)
    for (int i = 0; i < length; ++i) {
        double mix = 0.0;
        for (auto& voice : voices) {
            bool gate = ((voice.control & 0x01) != 0);
            voice.env.Update(gate, sampleRate);
            mix += voice.Next(sampleRate);
        }

        double vol = currentVolume / 15.0;
        mix *= vol;

        mix = std::min(mix, 1.0);
        mix = std::max(mix, -1.0);

        buffer[i] = static_cast<int16_t>(mix * 5000.0);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
            rate = 1.0 / (ATTACK_RATES.at(attackRate) * sampleRate);
            level += rate;
            if (level >= 1.0) {
                level = 1.0;
                state = DECAY;
            }
            break;
        case DECAY:
            rate = 1.0 / (DECAY_RELEASE_RATES.at(decayRate) * sampleRate);
            level -= rate;
            if (level <= sustainLevel) {
                level = sustainLevel;
                state = SUSTAIN;
            }
            break;
        case SUSTAIN:
            if (level > sustainLevel) {
                rate = 1.0 / (DECAY_RELEASE_RATES.at(decayRate) * sampleRate);
                level -= rate;
            } else {
                level = sustainLevel;
            }
            break;
        case RELEASE:
            rate = 1.0 / (DECAY_RELEASE_RATES.at(releaseRate) * sampleRate);
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
    if (frequency == 0) {
        return 0.0;
    }

    double step24 = frequency * (SID_CLOCK / sampleRate);
    accumulator = (accumulator + static_cast<uint32_t>(step24)) & 0xFFFFFF;

    double output = 0.0;

    if ((control & 0x80) != 0) {  // Noise
        // Xorshift32
        noiseShift ^= noiseShift << 13;
        noiseShift ^= noiseShift >> 17;
        noiseShift ^= noiseShift << 5;
        output = ((static_cast<double>(noiseShift) / 0xFFFFFFFF) * 2.0) - 1.0;
    } else if ((control & 0x10) != 0) {  // Triangle
        uint32_t temp = accumulator;
        if ((temp & 0x800000) != 0) {
            temp ^= 0xFFFFFF;
        }
        output = (static_cast<double>(temp) / 0x400000) - 1.0;
    } else if ((control & 0x20) != 0) {  // Sawtooth
        output = (static_cast<double>(accumulator) / 0x800000) - 1.0;
    } else if ((control & 0x40) != 0) {  // Pulse
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
    out.write(reinterpret_cast<const char*>(&state), sizeof(state));                // NOLINT
    out.write(reinterpret_cast<const char*>(&level), sizeof(level));                // NOLINT
    out.write(reinterpret_cast<const char*>(&attackRate), sizeof(attackRate));      // NOLINT
    out.write(reinterpret_cast<const char*>(&decayRate), sizeof(decayRate));        // NOLINT
    out.write(reinterpret_cast<const char*>(&sustainLevel), sizeof(sustainLevel));  // NOLINT
    out.write(reinterpret_cast<const char*>(&releaseRate), sizeof(releaseRate));    // NOLINT
    return out.good();
}

bool ADSREnvelope::LoadState(std::istream& inStream) {
    inStream.read(reinterpret_cast<char*>(&state), sizeof(state));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&level), sizeof(level));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&attackRate), sizeof(attackRate));      // NOLINT
    inStream.read(reinterpret_cast<char*>(&decayRate), sizeof(decayRate));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&sustainLevel), sizeof(sustainLevel));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&releaseRate), sizeof(releaseRate));    // NOLINT
    return inStream.good();
}

bool Oscillator::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&accumulator), sizeof(accumulator));  // NOLINT
    out.write(reinterpret_cast<const char*>(&frequency), sizeof(frequency));      // NOLINT
    out.write(reinterpret_cast<const char*>(&pulseWidth), sizeof(pulseWidth));    // NOLINT
    out.write(reinterpret_cast<const char*>(&control), sizeof(control));          // NOLINT
    out.write(reinterpret_cast<const char*>(&noiseShift), sizeof(noiseShift));    // NOLINT
    env.SaveState(out);
    return out.good();
}

bool Oscillator::LoadState(std::istream& inStream) {
    inStream.read(reinterpret_cast<char*>(&accumulator), sizeof(accumulator));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&frequency), sizeof(frequency));      // NOLINT
    inStream.read(reinterpret_cast<char*>(&pulseWidth), sizeof(pulseWidth));    // NOLINT
    inStream.read(reinterpret_cast<char*>(&control), sizeof(control));          // NOLINT
    inStream.read(reinterpret_cast<char*>(&noiseShift), sizeof(noiseShift));    // NOLINT
    env.LoadState(inStream);
    return inStream.good();
}

bool SID::SaveState(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(sidMutex);
    out.write(reinterpret_cast<const char*>(registers.data()),  // NOLINT
              static_cast<std::streamsize>(registers.size()));
    for (const auto& voice : voices) {
        voice.SaveState(out);
    }
    out.write(reinterpret_cast<const char*>(&volumeRegister), sizeof(volumeRegister));    // NOLINT
    out.write(reinterpret_cast<const char*>(&soundEnabled), sizeof(soundEnabled));        // NOLINT
    out.write(reinterpret_cast<const char*>(&emulationPaused), sizeof(emulationPaused));  // NOLINT
    return out.good();
}

bool SID::LoadState(std::istream& inStream) {
    std::lock_guard<std::mutex> lock(sidMutex);
    inStream.read(reinterpret_cast<char*>(registers.data()), static_cast<std::streamsize>(registers.size()));  // NOLINT
    for (auto& voice : voices) {
        voice.LoadState(inStream);
    }
    inStream.read(reinterpret_cast<char*>(&volumeRegister), sizeof(volumeRegister));    // NOLINT
    inStream.read(reinterpret_cast<char*>(&soundEnabled), sizeof(soundEnabled));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&emulationPaused), sizeof(emulationPaused));  // NOLINT
    UpdateAudioState();
    return inStream.good();
}

}  // namespace Hardware
