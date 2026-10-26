#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#ifndef TARGET_WASM
#include "Hardware/Audio/AudioRecorder.h"
#endif
#include "Hardware/Audio/SIDResampler.h"

namespace Hardware {

class SIDAudioSink {
   public:
    static constexpr size_t AUDIO_SNAPSHOT_SIZE = 256;

    SIDAudioSink();
    ~SIDAudioSink();

    SIDAudioSink(const SIDAudioSink&) = delete;
    SIDAudioSink& operator=(const SIDAudioSink&) = delete;
    SIDAudioSink(SIDAudioSink&&) = delete;
    SIDAudioSink& operator=(SIDAudioSink&&) = delete;

    void Init(int sampleRate = 48000);
    void Close();
    void Reset();

    void FlushAvailableOutput(SIDResampler& resampler, bool emulationPaused, bool soundEnabled);

    void StartRecording(const std::string& filename, bool emulationPaused, bool soundEnabled);
    void StopRecording();
    [[nodiscard]] bool IsRecording() const;

    void SetAudioCallback(std::function<void(const int16_t*, int)> callback);
    void ClearAudioCallback();
    [[nodiscard]] bool HasAudioCallback() const;

    void GetAudioSnapshot(std::span<float> outBuffer) const;

    void UpdateAudioState(bool soundEnabled, bool emulationPaused);
    void ClearAudioStream();
    void OnResumePlayback(bool soundEnabled);

    [[nodiscard]] int GetSampleRate() const { return sampleRate; }

   private:
    SDL_AudioStream* audioStream = nullptr;
    int sampleRate = 48000;

    std::vector<int16_t> sampleBuffer;

#ifndef TARGET_WASM
    std::unique_ptr<AudioRecorder> recorder;
#endif
    std::string pendingFilename;
    std::function<void(const int16_t*, int)> audioCallback;

    mutable std::array<float, AUDIO_SNAPSHOT_SIZE> visualizerBuffer{};
    std::atomic<size_t> visualizerWriteIndex{0};
};

}  // namespace Hardware
