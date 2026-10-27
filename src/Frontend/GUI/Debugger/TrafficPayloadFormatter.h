#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace GUI {

class TrafficPayloadFormatter {
public:
    static std::vector<std::string> FormatHexDumpLines(std::string_view data, size_t bytesPerLine = 16);
    static std::string FormatHexDump(std::string_view data, size_t bytesPerLine = 16);
    static void DrawHexDump(std::string_view data, size_t bytesPerLine = 16);
};

}  // namespace GUI
