#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <array>
#include <vector>
#include <atomic>
#include <cstdint>
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVAudioFifo;

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
                     const AudioParams& audioParams);
    
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
    bool SetupVideoStream(AVStream** outStream, AVCodecContext** outCodecCtx, int texWidth, int texHeight, bool isRaw);
    bool SetupAudioStream(const AudioParams& params);


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
    
    std::thread workerThread;
    std::mutex queueMutex;
    std::condition_variable queueCondVar;
    
    std::queue<VideoFrameData> videoQueue;
    std::queue<std::vector<float>> audioQueue;
    
    std::atomic<bool> isRunning{false};
    
    int64_t nextVideoPts = 0;
    int64_t nextAudioPts = 0;

    std::array<uint32_t, 2> pboRaw = {0, 0};
    std::array<uint32_t, 2> pboProcessed = {0, 0};
    int pboIndex = 0;
    int pboWarmupFrames = 0;
};
