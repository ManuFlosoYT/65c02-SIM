#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Control {
struct AppState;
}

namespace GUI::MIDI {
class MIDIAudioEngine;
class MIDIProjectSession;

class MIDICompilerPipeline {
public:
    static std::string SanitizeCleanStem(const std::string& path);

#ifndef TARGET_WASM
    static bool CompileMIDIProjectToBinary(
        MIDIProjectSession& session,
        Control::AppState& state,
        bool isNtsc,
        bool microDosSid,
        std::vector<std::uint8_t>& outBinary,
        std::string& outSavedPath);

    static bool HandleCompileAndRunROM(
        MIDIProjectSession& session,
        MIDIAudioEngine& audioEngine,
        Control::AppState& state);
#endif
};

} // namespace GUI::MIDI
