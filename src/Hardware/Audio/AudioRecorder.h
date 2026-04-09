#pragma once

#include <sndfile.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Hardware {

class AudioRecorder {
   public:
    AudioRecorder();
    ~AudioRecorder();

    AudioRecorder(const AudioRecorder&) = delete;
    AudioRecorder& operator=(const AudioRecorder&) = delete;

    AudioRecorder(AudioRecorder&&) = delete;
    AudioRecorder& operator=(AudioRecorder&&) = delete;

    bool Start(const std::string& filename, int sampleRate);

    void Stop();

    void PushAudio(const int16_t* data, size_t numSamples);

   private:
    void WorkerThread();

    SNDFILE* handle = nullptr;
    SF_INFO sfInfo{};

    // Thread-safe queue for audio chunks
    std::queue<std::vector<int16_t>> audioQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    
    std::thread worker;
    std::atomic<bool> running{false};
};

}  // namespace Hardware
