#ifndef TARGET_WASM
#include "MediaExporter.h"
#include <cstring>
#include <span>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
}

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
    this->rawWidth = rawW;
    this->rawHeight = rawH;
    this->processedWidth = processedW;
    this->processedHeight = processedH;
    this->recordingType = type;
    this->videoFormat = format;
    this->audioFormat = audioFormat;
    this->shouldRecordRaw = recordRaw;
    this->shouldRecordProcessed = recordProcessed;

    const char* formatName = (format == Control::VideoFormat::MP4) ? "mp4" : "matroska";
    avformat_alloc_output_context2(&fmtCtx, nullptr, formatName, filename.c_str());
    if (fmtCtx == nullptr) {
        std::cerr << "MediaExporter: Failed to allocate format context for " << formatName << "\n";
        return false;
    }

    if (recordingType != Control::RecordingType::Audio) {
        if (shouldRecordRaw) {
            if (!SetupVideoStream(&videoStreamRaw, &videoCodecCtxRaw, rawWidth, rawHeight, "Raw Video", true)) {
                return false;
            }
        }

        if (shouldRecordProcessed) {
            std::string trackName = (recordingType == Control::RecordingType::SIDWindow) ? "SID Viewer" : "Processed Video";
            if (!SetupVideoStream(&videoStreamProcessed, &videoCodecCtxProcessed, processedWidth, processedHeight, trackName, !shouldRecordRaw)) {
                return false;
            }
        }
    }

    if (!SetupAudioStream(audioParams)) {
        return false;
    }

    if (((fmtCtx->oformat->flags & AVFMT_NOFILE) == 0) &&
        (avio_open(&fmtCtx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0)) {
        std::cerr << "MediaExporter: Could not open output file\n";
        return false;
    }

    if (avformat_write_header(fmtCtx, nullptr) < 0) {
        std::cerr << "MediaExporter: Error writing header\n";
        return false;
    }

    if (shouldRecordRaw) {
        SetupSwsContext(&swsCtxRaw, rawWidth, rawHeight, videoCodecCtxRaw);
        SetupPBOs(pboRaw, rawWidth, rawHeight);
    }

    if (shouldRecordProcessed) {
        SetupSwsContext(&swsCtxProcessed, processedWidth, processedHeight, videoCodecCtxProcessed);
        SetupPBOs(pboProcessed, processedWidth, processedHeight);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    pboWarmupFrames = 0;
    isRunning = true;
    workerThread = std::thread(&MediaExporter::WorkerLoop, this);

    return true;
}

void MediaExporter::SetupPBOs(std::array<uint32_t, 2>& pbos, int width, int height) {
    glGenBuffers(2, pbos.data());
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos.at(i));
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(width) * height * 4, nullptr, GL_STREAM_READ);
    }
}

void MediaExporter::SetupSwsContext(SwsContext** ctx, int srcW, int srcH, AVCodecContext* codecCtx) {
    *ctx = sws_getContext(srcW, srcH, AV_PIX_FMT_RGBA,
                           codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                           SWS_POINT, nullptr, nullptr, nullptr);
}

bool MediaExporter::SetupVideoStream(AVStream** outStream, AVCodecContext** outCodecCtx, int texWidth, int texHeight, const std::string& trackName, bool isDefault) {
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (codec == nullptr) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }

    if (codec == nullptr) {
        std::cerr << "MediaExporter: H.264 encoder not found\n";
        return false;
    }

    *outStream = avformat_new_stream(fmtCtx, codec);
    if (*outStream == nullptr) {
        return false;
    }

    *outCodecCtx = avcodec_alloc_context3(codec);
    if (*outCodecCtx == nullptr) {
        return false;
    }

    // libx264 requires dimensions divisible by 2
    int encW = (texWidth  % 2 == 0) ? texWidth  : texWidth  + 1;
    int encH = (texHeight % 2 == 0) ? texHeight : texHeight + 1;

    (*outCodecCtx)->width = encW;
    (*outCodecCtx)->height = encH;
    (*outCodecCtx)->time_base = {.num = 1, .den = 60};
    (*outStream)->time_base = (*outCodecCtx)->time_base;
    (*outCodecCtx)->framerate = {.num = 60, .den = 1};
    (*outCodecCtx)->gop_size = 12;
    (*outCodecCtx)->max_b_frames = 2;
    (*outCodecCtx)->pix_fmt = AV_PIX_FMT_YUV420P;

    if ((fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
        (*outCodecCtx)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    av_dict_set(&(*outStream)->metadata, "title", trackName.c_str(), 0);
    (*outStream)->disposition = isDefault ? AV_DISPOSITION_DEFAULT : 0;

    AVDictionary* param = nullptr;
    av_dict_set(&param, "crf", "23", 0);
    av_dict_set(&param, "preset", "slow", 0);
    
    if (trackName == "SID Viewer") {
        av_dict_set(&param, "tune", "stillimage", 0);
    }

    if (avcodec_open2(*outCodecCtx, codec, &param) < 0) {
        std::cerr << "MediaExporter: avcodec_open2 failed for " << trackName << '\n';
        av_dict_free(&param);
        avcodec_free_context(outCodecCtx);
        return false;
    }
    av_dict_free(&param);

    avcodec_parameters_from_context((*outStream)->codecpar, *outCodecCtx);
    return true;
}

bool MediaExporter::SetupAudioStream(const AudioParams& params) {
    const AVCodec* codec = nullptr;
    if (audioFormat == Control::AudioFormat::FLAC) {
        codec = avcodec_find_encoder(AV_CODEC_ID_FLAC);
    } else {
        codec = avcodec_find_encoder_by_name("libopus");
        if (codec == nullptr) {
            codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
        }
    }

    if (codec == nullptr) {
        std::cerr << "MediaExporter: Audio encoder not found\n";
        return false;
    }

    audioStream = avformat_new_stream(fmtCtx, codec);
    if (audioStream == nullptr) {
        return false;
    }

    audioCodecCtx = avcodec_alloc_context3(codec);
    if (audioCodecCtx == nullptr) {
        return false;
    }

    audioCodecCtx->sample_rate = params.sampleRate;
    audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;

    // Opus prefers Float; FLAC uses S16 by default in this implementation
    if (audioFormat == Control::AudioFormat::OPUS) {
        audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLT;
        audioCodecCtx->bit_rate = 320000;
    } else {
        audioCodecCtx->compression_level = 5;
    }

    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    av_channel_layout_copy(&audioCodecCtx->ch_layout, &stereo);
    audioCodecCtx->time_base = {.num = 1, .den = params.sampleRate};
    audioStream->time_base = audioCodecCtx->time_base;

    if ((fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
        audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(audioCodecCtx, codec, nullptr) < 0) {
        std::cerr << "MediaExporter: avcodec_open2 failed for audio encoder\n";
        avcodec_free_context(&audioCodecCtx);
        return false;
    }

    avcodec_parameters_from_context(audioStream->codecpar, audioCodecCtx);

    audioFifo = av_audio_fifo_alloc(audioCodecCtx->sample_fmt, audioCodecCtx->ch_layout.nb_channels, 1);
    return true;
}

void MediaExporter::PushFrames(uint32_t texRaw, uint32_t texProcessed, bool emulationPaused) {
    if (!isRunning || emulationPaused) {
        return;
    }

    int nextIndex = (pboIndex + 1) % 2;

    if (shouldRecordRaw) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRaw.at(pboIndex));
        glBindTexture(GL_TEXTURE_2D, texRaw);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    std::vector<uint8_t> processedPixels;
    if (shouldRecordProcessed) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(pboIndex));
        glBindTexture(GL_TEXTURE_2D, texProcessed);
        
        GLint actualProcW = 0;
        GLint actualProcH = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &actualProcW);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actualProcH);

        if (actualProcW > 0 && actualProcH > 0 && actualProcW < 8192 && actualProcH < 8192) {
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    if (pboWarmupFrames >= 1) {
        VideoFrameData frameData;
        frameData.pts = nextVideoPts++;
        bool hasData = false;

        if (shouldRecordRaw) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRaw.at(nextIndex));
            const auto* ptrRaw = static_cast<const uint8_t*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
            if (ptrRaw != nullptr) {
                frameData.rawW = rawWidth;
                frameData.rawH = rawHeight;
                size_t szRaw = static_cast<size_t>(rawWidth) * static_cast<size_t>(rawHeight) * 4U;
                frameData.rawPixels.resize(szRaw);
                std::memcpy(frameData.rawPixels.data(), ptrRaw, szRaw);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                hasData = true;
            }
        }

        if (shouldRecordProcessed) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(nextIndex));
            const auto* ptrProc = static_cast<const uint8_t*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
            if (ptrProc != nullptr) {
                // We need the dimensions from when the command was issued
                // For simplicity, let's assume they haven't changed or use stored ones
                frameData.processedW = processedWidth;
                frameData.processedH = processedHeight;
                size_t szProc = static_cast<size_t>(processedWidth) * static_cast<size_t>(processedHeight) * 4U;
                frameData.processedPixels.resize(szProc);
                std::memcpy(frameData.processedPixels.data(), ptrProc, szProc);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                hasData = true;
            }
        }

        if (hasData) {
            if (videoQueue.push(std::move(frameData))) {
                wakeUpCount.fetch_add(1, std::memory_order_release);
                wakeUpCount.notify_one();
            }
        }
    } else {
        ++pboWarmupFrames;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    pboIndex = nextIndex;
}

std::vector<uint8_t> MediaExporter::ReadTextureSynchronous(uint32_t tex, int width, int height) {
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return pixels;
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

void MediaExporter::ProcessVideoFrame(const VideoFrameData& vData, AVFrame* frameRaw, AVFrame* frameProcessed) {
    av_frame_make_writable(frameRaw);
    av_frame_make_writable(frameProcessed);

    const std::array<const uint8_t*, 1> rawSrc = {vData.rawPixels.data()};
    const std::array<const uint8_t*, 1> procSrc = {vData.processedPixels.data()};
    const std::array<int, 1> rawStride = {vData.rawW * 4};
    const std::array<int, 1> procStride = {vData.processedW * 4};

    if (shouldRecordRaw) {
        sws_scale(swsCtxRaw, rawSrc.data(), rawStride.data(), 0, vData.rawH, frameRaw->data, frameRaw->linesize);

        // Zero out any Y padding rows (encoder height > texture height)
        for (int row = vData.rawH; row < videoCodecCtxRaw->height; ++row) {
            uint8_t* rowPtr = std::next(frameRaw->data[0], static_cast<std::ptrdiff_t>(row) * frameRaw->linesize[0]);
            memset(rowPtr, 16, static_cast<size_t>(videoCodecCtxRaw->width));
        }
        frameRaw->pts = vData.pts;
        EncodeVideoFrame(frameRaw, videoCodecCtxRaw, videoStreamRaw);
    }

    if (shouldRecordProcessed) {
        // Recreate the sws context if the processed texture dimensions changed
        if (vData.processedW != processedWidth || vData.processedH != processedHeight) {
            if (swsCtxProcessed != nullptr) {
                sws_freeContext(swsCtxProcessed);
            }
            swsCtxProcessed = sws_getContext(vData.processedW, vData.processedH, AV_PIX_FMT_RGBA, videoCodecCtxProcessed->width,
                                             videoCodecCtxProcessed->height, videoCodecCtxProcessed->pix_fmt, SWS_POINT, nullptr,
                                             nullptr, nullptr);
            processedWidth = vData.processedW;
            processedHeight = vData.processedH;
        }

        sws_scale(swsCtxProcessed, procSrc.data(), procStride.data(), 0, vData.processedH, frameProcessed->data,
                  frameProcessed->linesize);
        frameProcessed->pts = vData.pts;
        EncodeVideoFrame(frameProcessed, videoCodecCtxProcessed, videoStreamProcessed);
    }
}

void MediaExporter::ProcessAudioData(const std::vector<float>& aData, AVFrame* audioFrame) {
    int codecChannels = audioCodecCtx->ch_layout.nb_channels;
    int numSamplesPerChannel = static_cast<int>(aData.size());

    if (audioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLT) {
        std::vector<float> fData;
        fData.reserve(aData.size() * static_cast<size_t>(codecChannels));
        for (float sampleF : aData) {
            float clamped = std::max(-1.0F, std::min(1.0F, sampleF));
            for (int ch = 0; ch < codecChannels; ++ch) {
                fData.push_back(clamped);
            }
        }
        void* fifoPtr = fData.data();
        av_audio_fifo_write(audioFifo, &fifoPtr, numSamplesPerChannel);
    } else {
        std::vector<int16_t> s16Data;
        s16Data.reserve(aData.size() * static_cast<size_t>(codecChannels));
        for (float sampleF : aData) {
            float clamped = std::max(-1.0F, std::min(1.0F, sampleF));
            auto sampleS16 = static_cast<int16_t>(clamped * 32767.0F);
            for (int ch = 0; ch < codecChannels; ++ch) {
                s16Data.push_back(sampleS16);
            }
        }
        void* fifoPtr = s16Data.data();
        av_audio_fifo_write(audioFifo, &fifoPtr, numSamplesPerChannel);
    }

    int frameSize = audioCodecCtx->frame_size;
    if (frameSize == 0) {
        frameSize = 4608;
    }

    while (av_audio_fifo_size(audioFifo) >= frameSize) {
        av_frame_unref(audioFrame);
        audioFrame->nb_samples = frameSize;
        audioFrame->format = audioCodecCtx->sample_fmt;
        audioFrame->sample_rate = audioCodecCtx->sample_rate;
        av_channel_layout_copy(&audioFrame->ch_layout, &audioCodecCtx->ch_layout);
        av_frame_get_buffer(audioFrame, 0);

        void* audioDst = audioFrame->data[0];
        av_audio_fifo_read(audioFifo, &audioDst, frameSize);
        audioFrame->pts = nextAudioPts;
        nextAudioPts += frameSize;
        EncodeAudioFrame(audioFrame);
    }
}

void MediaExporter::WorkerLoop() {
    AVFrame* frameRaw = av_frame_alloc();
    AVFrame* frameProcessed = av_frame_alloc();
    if (shouldRecordRaw) {
        frameRaw->format = videoCodecCtxRaw->pix_fmt;
        frameRaw->width = videoCodecCtxRaw->width;
        frameRaw->height = videoCodecCtxRaw->height;
        av_frame_get_buffer(frameRaw, 32);
    }

    if (shouldRecordProcessed) {
        frameProcessed->format = videoCodecCtxProcessed->pix_fmt;
        frameProcessed->width = videoCodecCtxProcessed->width;
        frameProcessed->height = videoCodecCtxProcessed->height;
        av_frame_get_buffer(frameProcessed, 32);
    }

    AVFrame* audioFrame = av_frame_alloc();
    audioFrame->format = audioCodecCtx->sample_fmt;
    av_channel_layout_copy(&audioFrame->ch_layout, &audioCodecCtx->ch_layout);
    audioFrame->sample_rate = audioCodecCtx->sample_rate;

    while (isRunning || !videoQueue.empty() || !audioQueue.empty()) {
        VideoFrameData vData;
        std::vector<float> aData;

        bool hasVideo = videoQueue.pop(vData);
        bool hasAudio = audioQueue.pop(aData);

        if (!hasVideo && !hasAudio) {
            if (!isRunning) break;
            int val = wakeUpCount.load(std::memory_order_acquire);
            wakeUpCount.wait(val, std::memory_order_acquire);
            continue;
        }

        if (hasVideo) {
            ProcessVideoFrame(vData, frameRaw, frameProcessed);
        }

        if (hasAudio) {
            ProcessAudioData(aData, audioFrame);
        }
    }

    av_frame_free(&frameRaw);
    av_frame_free(&frameProcessed);
    av_frame_free(&audioFrame);
}

void MediaExporter::EncodeVideoFrame(AVFrame* frame, AVCodecContext* codecCtx, AVStream* stream) {
    if (avcodec_send_frame(codecCtx, frame) >= 0) {
        AVPacket* pkt = av_packet_alloc();
        while (avcodec_receive_packet(codecCtx, pkt) >= 0) {
            av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
            pkt->stream_index = stream->index;
            av_interleaved_write_frame(fmtCtx, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }
}

void MediaExporter::EncodeAudioFrame(AVFrame* frame) {
    if (avcodec_send_frame(audioCodecCtx, frame) >= 0) {
        AVPacket* pkt = av_packet_alloc();
        while (avcodec_receive_packet(audioCodecCtx, pkt) >= 0) {
            av_packet_rescale_ts(pkt, audioCodecCtx->time_base, audioStream->time_base);
            pkt->stream_index = audioStream->index;
            av_interleaved_write_frame(fmtCtx, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
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

    if (fmtCtx != nullptr) {
        if (videoCodecCtxRaw != nullptr) {
            EncodeVideoFrame(nullptr, videoCodecCtxRaw, videoStreamRaw);
        }
        if (videoCodecCtxProcessed != nullptr) {
            EncodeVideoFrame(nullptr, videoCodecCtxProcessed, videoStreamProcessed);
        }

        if (audioCodecCtx != nullptr) {
            AVPacket* pkt = av_packet_alloc();
            avcodec_send_frame(audioCodecCtx, nullptr);
            while (avcodec_receive_packet(audioCodecCtx, pkt) >= 0) {
                av_packet_rescale_ts(pkt, audioCodecCtx->time_base, audioStream->time_base);
                pkt->stream_index = audioStream->index;
                av_interleaved_write_frame(fmtCtx, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);
        }

        av_write_trailer(fmtCtx);

        if ((fmtCtx->oformat->flags & AVFMT_NOFILE) == 0) {
            avio_closep(&fmtCtx->pb);
        }
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }

    if (videoCodecCtxRaw != nullptr)       { avcodec_free_context(&videoCodecCtxRaw); }
    if (videoCodecCtxProcessed != nullptr) { avcodec_free_context(&videoCodecCtxProcessed); }
    if (audioCodecCtx != nullptr)          { avcodec_free_context(&audioCodecCtx); }

    if (swsCtxRaw != nullptr)       { sws_freeContext(swsCtxRaw);       swsCtxRaw = nullptr; }
    if (swsCtxProcessed != nullptr) { sws_freeContext(swsCtxProcessed); swsCtxProcessed = nullptr; }

    if (audioFifo != nullptr) { av_audio_fifo_free(audioFifo); audioFifo = nullptr; }

    if (pboRaw[0] != 0U) {
        glDeleteBuffers(2, pboRaw.data());
        pboRaw.fill(0U);
    }
    if (pboProcessed[0] != 0U) {
        glDeleteBuffers(2, pboProcessed.data());
        pboProcessed.fill(0U);
    }
}
#endif
