#pragma once

#include <glad/gl.h>

#include <atomic>
#include <string>

#include "Frontend/GUI/Video/CRTFilter.h"
#include "Hardware/Core/Emulator.h"

namespace Control {

using namespace Core;
using namespace Hardware;

struct AppState {
    Emulator emulator;

    // ROM
    std::string bin;
    bool romLoaded = false;

    // Emulation
    int instructionsPerFrame = 1000000;
    float ipsLogScale = 6.0f;
    bool gpuEnabled = false;
    bool cycleAccurate = true;
    bool forceLoadSaveState = false;
    bool autoReload = true;

    // Debugger
    bool debuggerOpen = false;
    // 0=Disassembly, 1=Profiler, 2=Debugger, 3=Memory Layout
    int debuggerMode = 0;

    // Update
    std::atomic<bool> updateAvailable{false};
    std::string latestVersionTag;

    // VRAM texture (OpenGL)
    GLuint vramTexture = 0;
    GLuint profilerTexture = 0;
    GLuint layoutTexture = 0;

    // CRT filters - The Essentials
    bool crtScanlines = true;
    bool crtInterlacing = true;
    bool crtCurvature = true;
    bool crtChromatic = true;
    bool crtBlur = true;

    // CRT filters - Screen Physicality
    bool crtShadowMask = true;
    bool crtVignette = true;
    bool crtCornerRounding = true;
    bool crtGlassGlare = true;

    // CRT filters - Signal & Analog Imperfections
    bool crtColorBleeding = true;
    bool crtNoise = true;
    bool crtVSyncJitter = true;
    bool crtPhosphorDecay = true;

    // CRT filters - Lighting
    bool crtBloom = true;

    float crtTime = 0.0f;
    GUI::CRTFilter crtFilter;

    // Last rendered display texture (post-CRT) for capture
    GLuint lastDisplayTex = 0;
    int lastDisplayW = 0;
    int lastDisplayH = 0;
};

}  // namespace Control
