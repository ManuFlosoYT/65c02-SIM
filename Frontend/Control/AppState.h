#pragma once

#include <glad/gl.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <array>

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

enum class RecordingType : std::uint8_t {
    Audio = 0,
    Video = 1,
    SIDWindow = 2
};

enum class VideoFormat : std::uint8_t { MKV, MP4 };
enum class AudioFormat : std::uint8_t { FLAC, OPUS };

struct RecordingSettings {
    RecordingType type = RecordingType::Audio;
    VideoFormat format = VideoFormat::MKV;
    AudioFormat audioFormat = AudioFormat::FLAC;
    bool recordRaw = true;
    bool recordProcessed = true;
    bool showPopup = false;
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
    RecordingSettings recordingSettings;
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
    bool ghosting = true;
    bool halation = true;
    bool moire = true;
    float gamma = 2.75F;
    float time = 0.0F;
};

struct RenderState {
    GLuint vramTexture = 0;
    GLuint profilerTexture = 0;
    GLuint layoutTexture = 0;
    GLuint sidTexture = 0;
    GLuint sidFBO = 0;
    GLuint lastDisplayTex = 0;
    int lastDisplayW = 0;
    int lastDisplayH = 0;
    std::array<float, 2> sidWindowPos = {0.0F, 0.0F};
    std::array<float, 2> sidWindowSize = {0.0F, 0.0F};
};

struct PopupState {
    bool sdCardDisabled = false;
    bool sdCardWebWarning = false;
};

struct SDKState {
    std::vector<std::string> roms;
    std::vector<std::string> midis;
    std::vector<std::string> vrams;
    bool loaded = false;
    bool showPopup = false;
};

struct IDEState {
    bool open = false;
    std::string code;
    std::string outputLog;
    std::string currentFilePath;
    std::vector<uint8_t> exportBinary;
    std::string exportDbgPath;
    bool isCMode = true;
    std::array<char, 65536> codeBuffer = {{0}};
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
    SDKState sdk;
    IDEState ide;

    GUI::CRTFilter crtFilter;
};

}  // namespace Control
