#pragma once

#include <sndfile.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "Hardware/Core/SPSCQueue.h"

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

    SPSCQueue<std::vector<int16_t>, 128> audioQueue;
    std::atomic<uint32_t> wakeUpCount{0};
    
    std::thread worker;
    std::atomic<bool> running{false};
};

}  // namespace Hardware
