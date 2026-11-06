#ifndef TARGET_WASM
#include "Frontend/MediaExporter.h"
#include <span>
#include <cmath>

MediaExporter::MediaExporter() = default;

MediaExporter::~MediaExporter() {
    Finalize();
}

bool MediaExporter::Initialize(const std::string& filename,
                                int rawW, int rawH,
                                int processedW, int processedH,
                                const AudioParams& audioParams,
                                Control::RecordingType type,
                                Control::VideoFormat format,
                                Control::AudioFormat audioFormat,
                                bool recordRaw,
                                bool recordProcessed) {
    encoder = std::make_unique<FFmpegEncoder>();
    if (!encoder->Initialize(filename, rawW, rawH, processedW, processedH,
                             audioParams, type, format, audioFormat,
                             recordRaw, recordProcessed)) {
        encoder.reset();
        return false;
    }

    if (type == Control::RecordingType::Audio) {
        grabber.Initialize(rawW, rawH, processedW, processedH, false, false);
    } else {
        grabber.Initialize(rawW, rawH, processedW, processedH, recordRaw, recordProcessed);
    }

    startTime = std::chrono::steady_clock::now();
    totalPausedDuration = std::chrono::steady_clock::duration::zero();
    wasPaused = false;
    lastRecordedPts = -1;
    isRunning = true;
    workerThread = std::thread(&MediaExporter::WorkerLoop, this);
    return true;
}

void MediaExporter::PushFrames(uint32_t texRaw, uint32_t texProcessed, bool emulationPaused) {
    if (!isRunning) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (emulationPaused) {
        if (!wasPaused) {
            lastPauseStart = now;
            wasPaused = true;
        }
        return;
    }

    if (wasPaused) {
        totalPausedDuration += (now - lastPauseStart);
        wasPaused = false;
    }

    auto elapsed = now - startTime - totalPausedDuration;
    int64_t targetPts = static_cast<int64_t>(std::round(std::chrono::duration<double>(elapsed).count() * 60.0));
    if (targetPts <= lastRecordedPts) {
        return;
    }

    VideoFrameData frameData;
    if (grabber.Capture(texRaw, texProcessed, frameData)) {
        frameData.pts = targetPts;
        if (videoQueue.push(std::move(frameData))) {
            lastRecordedPts = targetPts;
            wakeUpCount.fetch_add(1, std::memory_order_release);
            wakeUpCount.notify_one();
        }
    }
}

void MediaExporter::PushAudio(const float* samples, int count) {
    if (!isRunning) {
        return;
    }
    std::span<const float> audioSpan(samples, static_cast<size_t>(count));
    std::vector<float> audioData(audioSpan.begin(), audioSpan.end());
    if (audioQueue.push(std::move(audioData))) {
        wakeUpCount.fetch_add(1, std::memory_order_release);
        wakeUpCount.notify_one();
    }
}

void MediaExporter::WorkerLoop() {
    while (isRunning || !videoQueue.empty() || !audioQueue.empty()) {
        std::vector<float> aData;
        while (audioQueue.pop(aData)) {
            if (encoder != nullptr) {
                encoder->ProcessAudioData(aData);
            }
        }

        VideoFrameData vData;
        bool hasVideo = videoQueue.pop(vData);
        if (hasVideo && encoder != nullptr) {
            encoder->ProcessVideoFrame(vData);
        }

        if (!hasVideo && audioQueue.empty()) {
            if (!isRunning) {
                break;
            }
            int val = wakeUpCount.load(std::memory_order_acquire);
            wakeUpCount.wait(val, std::memory_order_acquire);
        }
    }

    if (encoder != nullptr) {
        encoder->Finalize();
    }
}

void MediaExporter::Finalize() {
    if (isRunning) {
        isRunning = false;
        wakeUpCount.fetch_add(1, std::memory_order_release);
        wakeUpCount.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    grabber.Cleanup();
    encoder.reset();
}
#endif
