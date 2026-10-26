#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Hardware/Core/SymbolTable.h"

namespace Control {

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
};

struct EmulationState {
    int instructionsPerFrame = 1000000;
    bool gpuEnabled = false;
    bool espEnabled = false;
    bool sdEnabled = false;
    bool cycleAccurate = true;
    bool forceLoadSaveState = false;
    bool autoReload = true;
    bool sdLatencyEnabled = false;
    bool ramPowerOnPattern = false;
    bool sidPhaseTrigger = true;
    bool sidADSRWarning = false;
    bool isRecordingVideo = false;
    std::string recordingVideoPath;
    RecordingSettings recordingSettings;
};

}  // namespace Control
