#include "Frontend/GUI/RecordingManager.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/Video/SIDWindowFrameGrabber.h"
#include "Frontend/MediaExporter.h"
#include <memory>
#include <vector>
#include <span>

using namespace Control;
using namespace Core;

namespace Frontend {

static constexpr int sidSampleRate = 48000;

static void HandleSIDWindowCapture(AppState& state) {
    SIDWindowFrameGrabber::CaptureSIDWindow(state);
}

static bool InitializeExporter(AppState& state, std::unique_ptr<MediaExporter>& mediaExporter) {
    auto& sid = state.emulator.GetSID();

    mediaExporter = std::make_unique<MediaExporter>();
    const MediaExporter::AudioParams audioParams{.sampleRate = sidSampleRate, .channels = 1, .bitDepth = 16};

    int videoW = 1280;
    int videoH = 720;
    int procW = 1280;
    int procH = 720;

    if (state.emulation.recordingSettings.type != RecordingType::SIDWindow) {
        videoW = GPU::VRAM_WIDTH;
        videoH = GPU::VRAM_HEIGHT;
        procW = state.render.lastDisplayW > 0 ? state.render.lastDisplayW : 600;
        procH = state.render.lastDisplayH > 0 ? state.render.lastDisplayH : 450;
    }

    bool initOk = mediaExporter->Initialize(state.emulation.recordingVideoPath, videoW, videoH,
                                             procW, procH, audioParams,
                                             state.emulation.recordingSettings.type,
                                             state.emulation.recordingSettings.format,
                                             state.emulation.recordingSettings.audioFormat, // Pass audioFormat
                                             state.emulation.recordingSettings.recordRaw,
                                             state.emulation.recordingSettings.recordProcessed);
    if (!initOk) {
        state.emulation.isRecordingVideo = false;
        mediaExporter.reset();
        return false;
    }

    sid.SetAudioCallback([mPtr = mediaExporter.get()](const int16_t* samples, int count) {
        std::vector<float> floatSamples(count);
        std::span<const int16_t> samplesSpan(samples, static_cast<std::size_t>(count));
        for (int idx = 0; idx < count; ++idx) {
            floatSamples[idx] = static_cast<float>(samplesSpan[idx]) / 32767.0F;
        }
        mPtr->PushAudio(floatSamples.data(), count);
    });
    return true;
}

static void HandleAutoStop(AppState& state) {
    // Auto-stop recording if emulation has halted or stopped after having run
    if (!state.emulation.isRecordingVideo) {
        return;
    }

    bool shouldAutoStop = (state.emulator.IsHalted() || !state.emulator.IsRunning());
    if (shouldAutoStop && state.emulator.GetTotalCycles() > 0) {
        state.emulation.isRecordingVideo = false;
        if (state.emulator.GetSID().IsRecording()) {
            state.emulator.GetSID().StopRecording();
        }
    }
}

void UpdateMediaRecording(AppState& state, std::unique_ptr<MediaExporter>& mediaExporter) {
    HandleAutoStop(state);

    if (state.emulation.isRecordingVideo && !mediaExporter) {
        InitializeExporter(state, mediaExporter);
    } else if (!state.emulation.isRecordingVideo && mediaExporter) {
        state.emulator.GetSID().ClearAudioCallback();
        mediaExporter->Finalize();
        mediaExporter.reset();
    }

    if (mediaExporter) {
        uint32_t processedTex = state.render.lastDisplayTex != 0 ? state.render.lastDisplayTex : state.render.vramTexture;
        uint32_t rawTex = state.render.vramTexture;

        if (state.emulation.recordingSettings.type == RecordingType::SIDWindow) {
            HandleSIDWindowCapture(state);
            rawTex = state.render.sidTexture;
            processedTex = state.render.sidTexture;
        }

        mediaExporter->PushFrames(rawTex, processedTex, state.emulator.IsPaused());
    }
}

}  // namespace Frontend
