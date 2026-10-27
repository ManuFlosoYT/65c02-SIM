#ifndef TARGET_WASM
#include "Hardware/Audio/AudioRecorder.h"

#include <iostream>
#include <span>

namespace Hardware {

AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder() {
    Stop();
}

bool AudioRecorder::Start(const std::string& filename, int sampleRate) {
    if (running) {
        Stop();
    }

    // Ensure SF_INFO is clean for writing
    sfInfo = {}; 
    if (filename.ends_with(".ogg")) {
        sfInfo.format = SF_FORMAT_OGG | SF_FORMAT_OPUS;
    } else {
        sfInfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    }
    sfInfo.channels = 1;
    sfInfo.samplerate = sampleRate;

    if (sf_format_check(&sfInfo) == 0) {
        return false;
    }

    handle = sf_open(filename.c_str(), SFM_WRITE, &sfInfo);
    if (handle == nullptr) {
        std::cerr << "AudioRecorder: Failed to open file " << filename << ": " << sf_strerror(nullptr) << '\n';
        return false;
    }

    running = true;
    worker = std::thread(&AudioRecorder::WorkerThread, this);
    return true;
}

void AudioRecorder::Stop() {
    if (!running) {
        return;
    }

    running = false;
    wakeUpCount.fetch_add(1, std::memory_order_release);
    wakeUpCount.notify_all();

    if (worker.joinable()) {
        worker.join();
    }

    if (handle != nullptr) {
        sf_close(handle);
        handle = nullptr;
    }

    std::vector<int16_t> chunk;
    while (audioQueue.pop(chunk)) {
    }
}

void AudioRecorder::PushAudio(const int16_t* data, size_t numSamples) {
    if (!running || data == nullptr || numSamples == 0) {
        return;
    }

    std::span<const int16_t> dataSpan(data, numSamples);
    std::vector<int16_t> chunk(dataSpan.begin(), dataSpan.end());
    if (audioQueue.push(std::move(chunk))) {
        wakeUpCount.fetch_add(1, std::memory_order_release);
        wakeUpCount.notify_one();
    }
}

void AudioRecorder::WorkerThread() {
    while (running || !audioQueue.empty()) {
        std::vector<int16_t> chunk;
        if (audioQueue.pop(chunk)) {
            if (!chunk.empty() && handle != nullptr) {
                sf_writef_short(handle, chunk.data(), static_cast<sf_count_t>(chunk.size()));
            }
        } else {
            if (!running) {
                break;
            }
            auto val = wakeUpCount.load(std::memory_order_acquire);
            wakeUpCount.wait(val, std::memory_order_acquire);
        }
    }
}

}  // namespace Hardware
#endif

