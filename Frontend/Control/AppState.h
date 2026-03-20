#pragma once

#include <glad/gl.h>

#include <atomic>
#include <cstdint>
#include <string>

#include "Frontend/Control/SymbolTable.h"
#include "Frontend/GUI/Video/CRTFilter.h"
#include "Hardware/Core/Emulator.h"

namespace Control {

using namespace Core;
using namespace Hardware;

struct ROMState {
    std::string bin;
    bool loaded = false;
    SymbolTable symbols;
    std::vector<uint8_t> data;
};

struct ScriptState {
    std::string path;
    bool loaded = false;
    bool showConsole = false;
};

struct EmulationState {
    int instructionsPerFrame = 1000000;
    float ipsLogScale = 6.0F;
    bool gpuEnabled = false;
    bool cycleAccurate = true;
    bool forceLoadSaveState = false;
    bool autoReload = true;
    bool isRecordingVideo = false;
    std::string recordingVideoPath;
};

enum class DebuggerMode : std::uint8_t {
    Disassembly = 0,
    Profiler = 1,
    Debugger = 2,
    MemoryLayout = 3
};

struct DebuggerState {
    bool open = false;
    DebuggerMode mode = DebuggerMode::Disassembly;
};

struct UpdateState {
    std::atomic<bool> available{false};
    std::string latestVersionTag;
};

struct CRTSettings {
    bool scanlines = true;
    bool interlacing = true;
    bool curvature = true;
    bool chromatic = true;
    bool blur = true;
    bool shadowMask = true;
    bool vignette = true;
    bool cornerRounding = true;
    bool glassGlare = true;
    bool colorBleeding = true;
    bool noise = true;
    bool vsyncJitter = true;
    bool phosphorDecay = true;
    bool bloom = true;
    float time = 0.0F;
};

struct RenderState {
    GLuint vramTexture = 0;
    GLuint profilerTexture = 0;
    GLuint layoutTexture = 0;
    GLuint lastDisplayTex = 0;
    int lastDisplayW = 0;
    int lastDisplayH = 0;
};

struct PopupState {
    bool sdCardDisabled = false;
    bool sdCardWebWarning = false;
};

struct AppState {
    Emulator emulator;

    ROMState rom;
    ScriptState script;
    EmulationState emulation;
    DebuggerState debugger;
    UpdateState update;
    CRTSettings crt;
    RenderState render;
    PopupState popups;

    GUI::CRTFilter crtFilter;
};

}  // namespace Control
