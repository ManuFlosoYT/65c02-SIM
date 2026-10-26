#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <array>
#include <cstdint>
#include <vector>
#include <memory>

#include "Frontend/Control/AppState.h"
#include "Frontend/GLPboFrameGrabber.h"
#include "Frontend/FFmpegEncoder.h"

class MediaExporter {
public:
    using AudioParams = FFmpegEncoder::AudioParams;

    MediaExporter();
    ~MediaExporter();

    MediaExporter(const MediaExporter&) = delete;
    MediaExporter& operator=(const MediaExporter&) = delete;
    MediaExporter(MediaExporter&&) = delete;
    MediaExporter& operator=(MediaExporter&&) = delete;

    bool Initialize(const std::string& filename,
                     int rawW, int rawH,
                     int processedW, int processedH,
                     const AudioParams& audioParams,
                     Control::RecordingType type,
                     Control::VideoFormat format,
                     Control::AudioFormat audioFormat,
                     bool recordRaw,
                     bool recordProcessed);

    void PushFrames(uint32_t texRaw, uint32_t texProcessed, bool emulationPaused);
    void PushAudio(const float* samples, int count);

    void Finalize();

private:
    void WorkerLoop();

    template <typename T, size_t N>
    class SPSCQueue {
        std::array<T, N> buffer;
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
    public:
        SPSCQueue() = default;
        bool push(T&& item) {
            auto current_tail = tail.load(std::memory_order_relaxed);
            auto next_tail = (current_tail + 1) % N;
            if (next_tail == head.load(std::memory_order_acquire)) {
                return false;
            }
            buffer[current_tail] = std::move(item);
            tail.store(next_tail, std::memory_order_release);
            return true;
        }
        bool pop(T& item) {
            auto current_head = head.load(std::memory_order_relaxed);
            if (current_head == tail.load(std::memory_order_acquire)) {
                return false;
            }
            item = std::move(buffer[current_head]);
            head.store((current_head + 1) % N, std::memory_order_release);
            return true;
        }
        [[nodiscard]] bool empty() const {
            return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
        }
    };

    GLPboFrameGrabber grabber;
    std::unique_ptr<FFmpegEncoder> encoder;
    std::thread workerThread;

    SPSCQueue<VideoFrameData, 240> videoQueue;
    SPSCQueue<std::vector<float>, 240> audioQueue;

    std::atomic<int> wakeUpCount{0};
    std::atomic<bool> isRunning{false};

    int64_t nextVideoPts = 0;
};
