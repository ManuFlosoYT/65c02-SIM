#ifndef TARGET_WASM
#include "Hardware/Audio/AudioRecorder.h"

#include <iostream>
#include <span>

namespace Hardware {

namespace {

#pragma pack(push, 1)
struct WavHeader {
    char riff[4]{'R', 'I', 'F', 'F'};
    uint32_t chunkSize{36};
    char wave[4]{'W', 'A', 'V', 'E'};
    char fmt[4]{'f', 'm', 't', ' '};
    uint32_t subchunk1Size{16};
    uint16_t audioFormat{1};
    uint16_t numChannels{1};
    uint32_t sampleRate{44100};
    uint32_t byteRate{88200};
    uint16_t blockAlign{2};
    uint16_t bitsPerSample{16};
    char data[4]{'d', 'a', 't', 'a'};
    uint32_t subchunk2Size{0};
};
#pragma pack(pop)
static_assert(sizeof(WavHeader) == 44);

}  // namespace

AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder() {
    Stop();
}

bool AudioRecorder::Start(const std::string& filename, int sampleRate) {
    if (running) {
        Stop();
    }

    if (outFile.is_open()) {
        outFile.close();
    }

    outFile.open(filename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "AudioRecorder: Failed to open file " << filename << '\n';
        return false;
    }

    totalDataBytes = 0;
    const uint32_t rate = static_cast<uint32_t>(sampleRate);
    const uint16_t channels = 1;
    const uint16_t bits = 16;
    const uint16_t align = channels * (bits / 8);
    const uint32_t byteRate = rate * align;

    WavHeader header;
    header.sampleRate = rate;
    header.byteRate = byteRate;
    header.blockAlign = align;
    header.bitsPerSample = bits;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!outFile) {
        outFile.close();
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

    if (outFile.is_open()) {
        const uint32_t fileSize = totalDataBytes + 36;
        outFile.seekp(4);
        outFile.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
        outFile.seekp(40);
        outFile.write(reinterpret_cast<const char*>(&totalDataBytes), sizeof(totalDataBytes));
        outFile.close();
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
            if (!chunk.empty() && outFile.is_open()) {
                const auto bytes = static_cast<std::streamsize>(chunk.size() * sizeof(int16_t));
                outFile.write(reinterpret_cast<const char*>(chunk.data()), bytes);
                totalDataBytes += static_cast<uint32_t>(bytes);
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

