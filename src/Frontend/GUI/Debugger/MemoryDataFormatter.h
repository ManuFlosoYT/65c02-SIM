#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Hardware/Core/Bus.h"

namespace GUI {

struct DetectedAsciiString {
    uint16_t address{0};
    int length{0};
    std::string text;
};

class MemoryDataFormatter {
public:
    static std::vector<DetectedAsciiString> DetectAsciiStrings(const Hardware::Bus& bus, uint16_t startAddr,
                                                               uint16_t endAddr, size_t minLength = 3);
    static std::string FormatAsCArray(const Hardware::Bus& bus, uint16_t startAddr, uint16_t endAddr,
                                      int mode, size_t maxPreviewItems = 24);
};

}  // namespace GUI
