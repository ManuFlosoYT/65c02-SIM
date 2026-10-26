#include "Hardware/Audio/SIDAudioSink.h"

#include <iostream>

namespace Hardware {

SIDAudioSink::SIDAudioSink() = default;

SIDAudioSink::~SIDAudioSink() {
    Close();
}

void SIDAudioSink::Close() {
    if (audioStream != nullptr) {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
    }
}

void SIDAudioSink::Init(int sampleRate) {
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

void SIDAudioSink::Reset() {
    sampleBuffer.clear();
    visualizerBuffer.fill(0.0F);
    if (audioStream != nullptr) {
        SDL_ClearAudioStream(audioStream);
    }
}

void SIDAudioSink::ClearAudioStream() {
    if (audioStream != nullptr) {
        SDL_ClearAudioStream(audioStream);
    }
    sampleBuffer.clear();
}

void SIDAudioSink::UpdateAudioState(bool soundEnabled, bool emulationPaused) {
    if (audioStream != nullptr) {
        SDL_AudioDeviceID devId = SDL_GetAudioStreamDevice(audioStream);
        if (!soundEnabled || emulationPaused) {
            SDL_PauseAudioDevice(devId);
        } else {
            SDL_ResumeAudioDevice(devId);
        }
    }
}

void SIDAudioSink::OnResumePlayback(bool soundEnabled) {
#ifndef TARGET_WASM
    if (soundEnabled && !pendingFilename.empty()) {
        auto newRecorder = std::make_unique<AudioRecorder>();
        if (newRecorder->Start(pendingFilename, sampleRate)) {
            recorder = std::move(newRecorder);
            pendingFilename.clear();
        }
    }
#else
    (void)soundEnabled;
#endif
}

void SIDAudioSink::FlushAvailableOutput(SIDResampler& resampler, bool emulationPaused, bool soundEnabled) {
    while (resampler.HasOutput()) {
        double finalMix = resampler.GetOutput();
        sampleBuffer.push_back(static_cast<int16_t>(finalMix * 18000.0));

        const size_t wIdx = visualizerWriteIndex.load(std::memory_order_relaxed);
        visualizerBuffer[wIdx % AUDIO_SNAPSHOT_SIZE] = static_cast<float>(finalMix);
        visualizerWriteIndex.store(wIdx + 1, std::memory_order_relaxed);

        if (sampleBuffer.size() >= 512) {
            if (audioStream != nullptr && !emulationPaused) {
                SDL_PutAudioStreamData(audioStream, sampleBuffer.data(),
                                       static_cast<int>(sampleBuffer.size() * sizeof(int16_t)));
            }
#ifndef TARGET_WASM
            if (recorder && soundEnabled && !emulationPaused) {
                recorder->PushAudio(sampleBuffer.data(), sampleBuffer.size());
            }
#endif
            if (audioCallback) {
                audioCallback(sampleBuffer.data(), static_cast<int>(sampleBuffer.size()));
            }
            sampleBuffer.clear();
        }
    }
}

void SIDAudioSink::StartRecording(const std::string& filename, bool emulationPaused, bool soundEnabled) {
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
    (void)emulationPaused;
    (void)soundEnabled;
#endif
}

void SIDAudioSink::StopRecording() {
    pendingFilename.clear();
#ifndef TARGET_WASM
    if (recorder) {
        recorder->Stop();
        recorder.reset();
    }
#endif
}

bool SIDAudioSink::IsRecording() const {
#ifndef TARGET_WASM
    return recorder != nullptr || !pendingFilename.empty();
#else
    return !pendingFilename.empty();
#endif
}

void SIDAudioSink::SetAudioCallback(std::function<void(const int16_t*, int)> callback) {
    audioCallback = std::move(callback);
}

void SIDAudioSink::ClearAudioCallback() {
    audioCallback = nullptr;
}

bool SIDAudioSink::HasAudioCallback() const {
    return static_cast<bool>(audioCallback);
}

void SIDAudioSink::GetAudioSnapshot(std::span<float> outBuffer) const {
    if (outBuffer.empty()) {
        return;
    }
    const size_t count = std::min(outBuffer.size(), AUDIO_SNAPSHOT_SIZE);
    const size_t writeIdx = visualizerWriteIndex.load(std::memory_order_relaxed);
    for (size_t i = 0; i < count; ++i) {
        const size_t bufIdx = (writeIdx + AUDIO_SNAPSHOT_SIZE - count + i) % AUDIO_SNAPSHOT_SIZE;
        outBuffer[i] = visualizerBuffer[bufIdx];
    }
    for (size_t i = count; i < outBuffer.size(); ++i) {
        outBuffer[i] = 0.0F;
    }
}

}  // namespace Hardware
