#pragma once

#include <GL/glew.h>

#include <atomic>
#include <string>

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

    // Update
    std::atomic<bool> updateAvailable{false};
    std::string latestVersionTag;

    // VRAM texture (OpenGL)
    GLuint vramTexture = 0;
};

}  // namespace Control
