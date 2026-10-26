#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "Frontend/Control/AppState.h"
#include "Frontend/GLPboFrameGrabber.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct SwsContext;
struct AVAudioFifo;

class FFmpegEncoder {
public:
    struct AudioParams {
        int sampleRate = 0;
        int channels = 0;
        int bitDepth = 0;
    };

    FFmpegEncoder();
    ~FFmpegEncoder();

    FFmpegEncoder(const FFmpegEncoder&) = delete;
    FFmpegEncoder& operator=(const FFmpegEncoder&) = delete;
    FFmpegEncoder(FFmpegEncoder&&) = delete;
    FFmpegEncoder& operator=(FFmpegEncoder&&) = delete;

    bool Initialize(const std::string& filename,
                    int rawW, int rawH,
                    int processedW, int processedH,
                    const AudioParams& audioParams,
                    Control::RecordingType type,
                    Control::VideoFormat format,
                    Control::AudioFormat audioFormat,
                    bool recordRaw,
                    bool recordProcessed);

    void ProcessVideoFrame(const VideoFrameData& vData);
    void ProcessAudioData(const std::vector<float>& aData);
    void Finalize();

private:
    void EncodeVideoFrame(AVFrame* frame, AVCodecContext* codecCtx, AVStream* stream);
    void EncodeAudioFrame(AVFrame* frame);
    bool SetupVideoStream(AVStream** outStream, AVCodecContext** outCodecCtx, int texWidth, int texHeight, const std::string& trackName, bool isDefault);
    bool SetupAudioStream(const AudioParams& params);
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

    AVFrame* frameRaw = nullptr;
    AVFrame* frameProcessed = nullptr;
    AVFrame* audioFrame = nullptr;

    int rawWidth = 0;
    int rawHeight = 0;
    int processedWidth = 0;
    int processedHeight = 0;

    int64_t nextAudioPts = 0;

    bool shouldRecordRaw = true;
    bool shouldRecordProcessed = true;
    bool headerWritten = false;
    Control::AudioFormat audioFormat = Control::AudioFormat::FLAC;
};
