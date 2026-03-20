#ifndef TARGET_WASM
#include "Hardware/Audio/AudioRecorder.h"

#include <iostream>
#include <span>

namespace Hardware {

AudioRecorder::AudioRecorder() {
    sfInfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    sfInfo.channels = 1;
}

AudioRecorder::~AudioRecorder() {
    Stop();
}

bool AudioRecorder::Start(const std::string& filename, int sampleRate) {
    if (running) {
        Stop();
    }

    sfInfo.samplerate = sampleRate;
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
    cv.notify_all();

    if (worker.joinable()) {
        worker.join();
    }

    if (handle != nullptr) {
        sf_close(handle);
        handle = nullptr;
    }

    // Clear remaining data in queue
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!audioQueue.empty()) {
        audioQueue.pop();
    }
}

void AudioRecorder::PushAudio(const int16_t* data, size_t numSamples) {
    if (!running || data == nullptr || numSamples == 0) {
        return;
    }

    // Copy data to a vector and push to queue
    std::span<const int16_t> dataSpan(data, numSamples);
    std::vector<int16_t> chunk(dataSpan.begin(), dataSpan.end());
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push(std::move(chunk));
    }
    cv.notify_one();
}

void AudioRecorder::WorkerThread() {
    while (running || !audioQueue.empty()) {
        std::vector<int16_t> chunk;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !audioQueue.empty() || !running; });
            
            if (audioQueue.empty() && !running) {
                break;
            }
            
            if (!audioQueue.empty()) {
                chunk = std::move(audioQueue.front());
                audioQueue.pop();
            }
        }

        if (!chunk.empty() && handle != nullptr) {
            sf_writef_short(handle, chunk.data(), static_cast<sf_count_t>(chunk.size()));
        }
    }
}

}  // namespace Hardware
#endif

