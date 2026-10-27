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
#include "Hardware/Core/SPSCQueue.h"

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

    GLPboFrameGrabber grabber;
    std::unique_ptr<FFmpegEncoder> encoder;
    std::thread workerThread;

    Hardware::SPSCQueue<VideoFrameData, 240> videoQueue;
    Hardware::SPSCQueue<std::vector<float>, 240> audioQueue;

    std::atomic<int> wakeUpCount{0};
    std::atomic<bool> isRunning{false};

    int64_t nextVideoPts = 0;
};
