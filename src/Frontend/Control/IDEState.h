#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Control {

enum class IDEMode : std::uint8_t {
    C = 0,
    ASM = 1,
    MicroDOS = 2,
    MIDI = 3
};

struct IDEState {
    bool open = false;
    std::string code;
    std::string outputLog;
    std::string currentFilePath;
    std::vector<uint8_t> exportBinary;
    bool isCMode = true;
    IDEMode mode = IDEMode::C;
};

}  // namespace Control
