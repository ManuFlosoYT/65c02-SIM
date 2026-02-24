#pragma once

#include <glad/gl.h>

#include <atomic>
#include <string>

#include "Frontend/GUI/Video/CRTFilter.h"
#include "Hardware/Emulator.h"

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
    bool cycleAccurate = true;  // Enabled by default
    bool ignoreSaveStateHash = false;

    // Debugger
    bool debuggerOpen = false;
    int debuggerMode = 0;  // 0=Debugger, 1=Disassembly, 2=Profiler

    // Update
    std::atomic<bool> updateAvailable{false};
    std::string latestVersionTag;

    // VRAM texture (OpenGL)
    GLuint vramTexture = 0;
    GLuint profilerTexture = 0;

    // CRT filters - The Essentials
    bool crtScanlines = false;
    bool crtCurvature = false;
    bool crtChromatic = false;
    bool crtBlur = false;

    // CRT filters - Screen Physicality
    bool crtShadowMask = false;
    bool crtVignette = false;
    bool crtCornerRounding = false;
    bool crtGlassGlare = false;

    // CRT filters - Signal & Analog Imperfections
    bool crtColorBleeding = false;
    bool crtNoise = false;
    bool crtVSyncJitter = false;
    bool crtPhosphorDecay = false;

    // CRT filters - Lighting
    bool crtBloom = false;

    float crtTime = 0.0f;
    GUI::CRTFilter crtFilter;
};

}  // namespace Control
