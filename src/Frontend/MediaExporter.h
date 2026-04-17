#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <array>
#include <cstdint>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVAudioFifo;

#include "Frontend/Control/AppState.h"

class MediaExporter {
public:
    struct AudioParams {
        int sampleRate;
        int channels;
        int bitDepth;
    };

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
    static std::vector<uint8_t> ReadTextureSynchronous(uint32_t tex, int width, int height);
    void PushAudio(const float* samples, int count);
    
    void Finalize();

private:
    struct VideoFrameData {
        std::vector<uint8_t> rawPixels;
        std::vector<uint8_t> processedPixels;
        int rawW = 0;
        int rawH = 0;
        int processedW = 0;
        int processedH = 0;
        int64_t pts;
    };

    void WorkerLoop();
    void EncodeVideoFrame(AVFrame* frame, AVCodecContext* codecCtx, AVStream* stream);
    void EncodeAudioFrame(AVFrame* frame);
    void FlushEncoder(AVCodecContext* codecCtx, AVStream* stream);
    void ProcessVideoFrame(const VideoFrameData& vData, AVFrame* frameRaw, AVFrame* frameProcessed);
    void ProcessAudioData(const std::vector<float>& aData, AVFrame* audioFrame);
    bool SetupVideoStream(AVStream** outStream, AVCodecContext** outCodecCtx, int texWidth, int texHeight, const std::string& trackName, bool isDefault);
    bool SetupAudioStream(const AudioParams& params);
    static void SetupPBOs(std::array<uint32_t, 2>& pbos, int width, int height);
    static void SetupSwsContext(SwsContext** ctx, int srcW, int srcH, AVCodecContext* codecCtx);


    AVFormatContext* fmtCtx = nullptr;
    
    AVStream* videoStreamRaw = nullptr;
    AVCodecContext* videoCodecCtxRaw = nullptr;
    
    AVStream* videoStreamProcessed = nullptr;
    AVCodecContext* videoCodecCtxProcessed = nullptr;
    
    AVStream* audioStream = nullptr;
    AVCodecContext* audioCodecCtx = nullptr;
    AVAudioFifo* audioFifo = nullptr;

    SwsContext* swsCtxRaw = nullptr;
    SwsContext* swsCtxProcessed = nullptr;

    int rawWidth = 0;
    int rawHeight = 0;
    int processedWidth = 0;
    int processedHeight = 0;
    
    template <typename T, size_t N>
    class SPSCQueue {
        std::array<T, N> buffer;
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
    public:
        // C++ requirement for vector init
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

    std::thread workerThread;
    
    SPSCQueue<VideoFrameData, 240> videoQueue;
    SPSCQueue<std::vector<float>, 240> audioQueue;
    
    std::atomic<int> wakeUpCount{0};
    std::atomic<bool> isRunning{false};
    
    int64_t nextVideoPts = 0;
    int64_t nextAudioPts = 0;

    bool shouldRecordRaw = true;
    bool shouldRecordProcessed = true;
    Control::RecordingType recordingType = Control::RecordingType::Video;
    Control::VideoFormat videoFormat = Control::VideoFormat::MKV;
    Control::AudioFormat audioFormat = Control::AudioFormat::FLAC;

    std::array<uint32_t, 2> pboRaw = {0, 0};
    std::array<uint32_t, 2> pboProcessed = {0, 0};
    int pboIndex = 0;
    int pboWarmupFrames = 0;
};
