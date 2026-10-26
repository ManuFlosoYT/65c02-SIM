#ifndef TARGET_WASM
#include "Frontend/FFmpegEncoder.h"
#include <iostream>
#include <cstring>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
}

FFmpegEncoder::FFmpegEncoder() = default;

FFmpegEncoder::~FFmpegEncoder() {
    Finalize();
}

bool FFmpegEncoder::Initialize(const std::string& filename,
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
    this->audioFormat = audioFormat;
    this->shouldRecordRaw = recordRaw;
    this->shouldRecordProcessed = recordProcessed;
    this->nextAudioPts = 0;

    const char* formatName = (format == Control::VideoFormat::MP4) ? "mp4" : "matroska";
    avformat_alloc_output_context2(&fmtCtx, nullptr, formatName, filename.c_str());
    if (fmtCtx == nullptr) {
        std::cerr << "MediaExporter: Failed to allocate format context for " << formatName << "\n";
        return false;
    }

    if (type != Control::RecordingType::Audio) {
        if (shouldRecordRaw) {
            if (!SetupVideoStream(&videoStreamRaw, &videoCodecCtxRaw, rawWidth, rawHeight, "Raw Video", true)) {
                return false;
            }
        }

        if (shouldRecordProcessed) {
            std::string trackName = (type == Control::RecordingType::SIDWindow) ? "SID Viewer" : "Processed Video";
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
    headerWritten = true;

    if (shouldRecordRaw) {
        SetupSwsContext(&swsCtxRaw, rawWidth, rawHeight, videoCodecCtxRaw);
        frameRaw = av_frame_alloc();
        if (frameRaw != nullptr) {
            frameRaw->format = videoCodecCtxRaw->pix_fmt;
            frameRaw->width = videoCodecCtxRaw->width;
            frameRaw->height = videoCodecCtxRaw->height;
            av_frame_get_buffer(frameRaw, 32);
        }
    }

    if (shouldRecordProcessed) {
        SetupSwsContext(&swsCtxProcessed, processedWidth, processedHeight, videoCodecCtxProcessed);
        frameProcessed = av_frame_alloc();
        if (frameProcessed != nullptr) {
            frameProcessed->format = videoCodecCtxProcessed->pix_fmt;
            frameProcessed->width = videoCodecCtxProcessed->width;
            frameProcessed->height = videoCodecCtxProcessed->height;
            av_frame_get_buffer(frameProcessed, 32);
        }
    }

    if (audioCodecCtx != nullptr) {
        audioFrame = av_frame_alloc();
        if (audioFrame != nullptr) {
            audioFrame->format = audioCodecCtx->sample_fmt;
            av_channel_layout_copy(&audioFrame->ch_layout, &audioCodecCtx->ch_layout);
            audioFrame->sample_rate = audioCodecCtx->sample_rate;
        }
    }

    return true;
}

void FFmpegEncoder::SetupSwsContext(SwsContext** ctx, int srcW, int srcH, AVCodecContext* codecCtx) {
    *ctx = sws_getContext(srcW, srcH, AV_PIX_FMT_RGBA,
                          codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                          SWS_POINT, nullptr, nullptr, nullptr);
}

bool FFmpegEncoder::SetupVideoStream(AVStream** outStream, AVCodecContext** outCodecCtx, int texWidth, int texHeight, const std::string& trackName, bool isDefault) {
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

    int encW = (texWidth % 2 == 0) ? texWidth : texWidth + 1;
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

bool FFmpegEncoder::SetupAudioStream(const AudioParams& params) {
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

void FFmpegEncoder::ProcessVideoFrame(const VideoFrameData& vData) {
    if (shouldRecordRaw && frameRaw != nullptr) {
        av_frame_make_writable(frameRaw);
        const std::array<const uint8_t*, 1> rawSrc = {vData.rawPixels.data()};
        const std::array<int, 1> rawStride = {vData.rawW * 4};
        sws_scale(swsCtxRaw, rawSrc.data(), rawStride.data(), 0, vData.rawH, frameRaw->data, frameRaw->linesize);

        for (int row = vData.rawH; row < videoCodecCtxRaw->height; ++row) {
            uint8_t* rowPtr = std::next(frameRaw->data[0], static_cast<std::ptrdiff_t>(row) * frameRaw->linesize[0]);
            memset(rowPtr, 16, static_cast<size_t>(videoCodecCtxRaw->width));
        }
        frameRaw->pts = vData.pts;
        EncodeVideoFrame(frameRaw, videoCodecCtxRaw, videoStreamRaw);
    }

    if (shouldRecordProcessed && frameProcessed != nullptr) {
        av_frame_make_writable(frameProcessed);
        const std::array<const uint8_t*, 1> procSrc = {vData.processedPixels.data()};
        const std::array<int, 1> procStride = {vData.processedW * 4};

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

void FFmpegEncoder::ProcessAudioData(const std::vector<float>& aData) {
    if (audioCodecCtx == nullptr || audioFifo == nullptr || audioFrame == nullptr) {
        return;
    }

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

void FFmpegEncoder::EncodeVideoFrame(AVFrame* frame, AVCodecContext* codecCtx, AVStream* stream) {
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

void FFmpegEncoder::EncodeAudioFrame(AVFrame* frame) {
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

void FFmpegEncoder::Finalize() {
    if (fmtCtx != nullptr) {
        if (videoCodecCtxRaw != nullptr) {
            EncodeVideoFrame(nullptr, videoCodecCtxRaw, videoStreamRaw);
        }
        if (videoCodecCtxProcessed != nullptr) {
            EncodeVideoFrame(nullptr, videoCodecCtxProcessed, videoStreamProcessed);
        }

        if (audioCodecCtx != nullptr) {
            if (audioFifo != nullptr) {
                int frameSize = audioCodecCtx->frame_size;
                if (frameSize == 0) {
                    frameSize = 4608;
                }
                while (av_audio_fifo_size(audioFifo) > 0) {
                    int remaining = av_audio_fifo_size(audioFifo);
                    int toRead = std::min(remaining, frameSize);
                    AVFrame* padFrame = av_frame_alloc();
                    if (padFrame == nullptr) {
                        break;
                    }
                    padFrame->nb_samples = frameSize;
                    padFrame->format = audioCodecCtx->sample_fmt;
                    padFrame->sample_rate = audioCodecCtx->sample_rate;
                    av_channel_layout_copy(&padFrame->ch_layout, &audioCodecCtx->ch_layout);
                    if (av_frame_get_buffer(padFrame, 0) >= 0) {
                        av_samples_set_silence(padFrame->data, 0, frameSize,
                                               audioCodecCtx->ch_layout.nb_channels,
                                               audioCodecCtx->sample_fmt);
                        void* audioDst = padFrame->data[0];
                        av_audio_fifo_read(audioFifo, &audioDst, toRead);
                        padFrame->pts = nextAudioPts;
                        nextAudioPts += frameSize;
                        EncodeAudioFrame(padFrame);
                    }
                    av_frame_free(&padFrame);
                }
            }
            EncodeAudioFrame(nullptr);
        }

        if (headerWritten) {
            av_write_trailer(fmtCtx);
            headerWritten = false;
        }

        if ((fmtCtx->oformat->flags & AVFMT_NOFILE) == 0) {
            avio_closep(&fmtCtx->pb);
        }
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }

    if (frameRaw != nullptr)       { av_frame_free(&frameRaw); }
    if (frameProcessed != nullptr) { av_frame_free(&frameProcessed); }
    if (audioFrame != nullptr)     { av_frame_free(&audioFrame); }

    if (videoCodecCtxRaw != nullptr)       { avcodec_free_context(&videoCodecCtxRaw); }
    if (videoCodecCtxProcessed != nullptr) { avcodec_free_context(&videoCodecCtxProcessed); }
    if (audioCodecCtx != nullptr)          { avcodec_free_context(&audioCodecCtx); }

    if (swsCtxRaw != nullptr)       { sws_freeContext(swsCtxRaw);       swsCtxRaw = nullptr; }
    if (swsCtxProcessed != nullptr) { sws_freeContext(swsCtxProcessed); swsCtxProcessed = nullptr; }

    if (audioFifo != nullptr) { av_audio_fifo_free(audioFifo); audioFifo = nullptr; }
}
#endif
