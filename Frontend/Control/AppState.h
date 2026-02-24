#pragma once

#include <glad/gl.h>

#include <atomic>
#include <string>

#include "Frontend/GUI/CRTFilter.h"
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

    // CRT filters
    bool crtScanlines = false;
    bool crtCurvature = false;
    bool crtChromatic = false;
    GUI::CRTFilter crtFilter;
};

}  // namespace Control
