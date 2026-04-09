#include "Frontend/UI/UIModules.h"
#include <glad/gl.h>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/MediaExporter.h"
#include <memory>
#include <vector>
#include <span>

using namespace Control;
using namespace Core;

namespace Frontend {

static constexpr int sidSampleRate = 48000;

static void HandleSIDWindowCapture(AppState& state) {
    const ImGuiIO& imguiIO = ImGui::GetIO();
    int screenHeightPixels = static_cast<int>(imguiIO.DisplaySize.y * imguiIO.DisplayFramebufferScale.y);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // Read from screen backbuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state.render.sidFBO);

    float scaleX = imguiIO.DisplayFramebufferScale.x;
    float scaleY = imguiIO.DisplayFramebufferScale.y;

    float windowX = state.render.sidWindowPos[0];
    float windowY = state.render.sidWindowPos[1];
    float windowW = state.render.sidWindowSize[0];
    float windowH = state.render.sidWindowSize[1];

    if (windowW <= 0 || windowH <= 0) {
        return;
    }

    // OpenGL coordinates start from bottom-left (pixels)
    int srcX0 = static_cast<int>(windowX * scaleX);
    int srcY0 = screenHeightPixels - static_cast<int>((windowY + windowH) * scaleY);
    int srcX1 = static_cast<int>((windowX + windowW) * scaleX);
    int srcY1 = screenHeightPixels - static_cast<int>(windowY * scaleY);

    // Calculate centered aspect ratio in 720p
    float targetW = 1280.0F;
    float targetH = 720.0F;
    float winAR = windowW / windowH;
    float targetAR = targetW / targetH;

    int drawW = 0;
    int drawH = 0;
    int offsetX = 0;
    int offsetY = 0;

    if (winAR > targetAR) {
        drawW = 1280;
        drawH = static_cast<int>(1280.0F / winAR);
        offsetY = (720 - drawH) / 2;
    } else {
        drawH = 720;
        drawW = static_cast<int>(720.0F * winAR);
        offsetX = (1280 - drawW) / 2;
    }

    // Clear FBO each frame to avoid artifacts in pillarbox areas
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // Blit and flip vertically for FFmpeg compatibility
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                        offsetX, offsetY + drawH, offsetX + drawW, offsetY, // Centered and Inverted Y
                        GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
