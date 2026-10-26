#ifndef TARGET_WASM
#include "Frontend/MediaExporter.h"
#include <span>

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

    grabber.Initialize(rawW, rawH, processedW, processedH, recordRaw, recordProcessed);

    nextVideoPts = 0;
    isRunning = true;
    workerThread = std::thread(&MediaExporter::WorkerLoop, this);
    return true;
}

void MediaExporter::PushFrames(uint32_t texRaw, uint32_t texProcessed, bool emulationPaused) {
    if (!isRunning || emulationPaused) {
        return;
    }

    VideoFrameData frameData;
    if (grabber.Capture(texRaw, texProcessed, frameData)) {
        frameData.pts = nextVideoPts++;
        if (videoQueue.push(std::move(frameData))) {
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
        VideoFrameData vData;
        std::vector<float> aData;

        bool hasVideo = videoQueue.pop(vData);
        bool hasAudio = audioQueue.pop(aData);

        if (!hasVideo && !hasAudio) {
            if (!isRunning) {
                break;
            }
            int val = wakeUpCount.load(std::memory_order_acquire);
            wakeUpCount.wait(val, std::memory_order_acquire);
            continue;
        }

        if (hasVideo && encoder != nullptr) {
            encoder->ProcessVideoFrame(vData);
        }

        if (hasAudio && encoder != nullptr) {
            encoder->ProcessAudioData(aData);
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
