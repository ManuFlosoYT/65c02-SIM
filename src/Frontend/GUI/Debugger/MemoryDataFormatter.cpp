#include "Frontend/GUI/Debugger/MemoryDataFormatter.h"

#include <algorithm>
#include <sstream>

namespace GUI {

std::vector<DetectedAsciiString> MemoryDataFormatter::DetectAsciiStrings(const Hardware::Bus& bus,
                                                                         uint16_t startAddr,
                                                                         uint16_t endAddr,
                                                                         size_t minLength) {
    if (startAddr > endAddr) {
        std::swap(startAddr, endAddr);
    }
    std::vector<DetectedAsciiString> results;
    int totalBytes = endAddr - startAddr + 1;
    int strStart = -1;
    std::string currentStr;

    for (int i = 0; i <= totalBytes; ++i) {
        uint8_t byte = (i < totalBytes) ? bus.ReadDirect(static_cast<Hardware::Word>(startAddr + i)) : 0;
        if (byte >= 32 && byte <= 126) {
            if (strStart == -1) {
                strStart = i;
            }
            currentStr.push_back(static_cast<char>(byte));
        } else {
            if (currentStr.size() >= minLength) {
                results.push_back({
                    .address = static_cast<uint16_t>(startAddr + strStart),
                    .length = static_cast<int>(currentStr.size()),
                    .text = std::move(currentStr),
                });
            }
            currentStr.clear();
            strStart = -1;
        }
    }
    return results;
}

std::string MemoryDataFormatter::FormatAsCArray(const Hardware::Bus& bus, uint16_t startAddr,
                                                uint16_t endAddr, int mode, size_t maxPreviewItems) {
    if (startAddr > endAddr) {
        std::swap(startAddr, endAddr);
    }
    int totalItems = (mode == 0) ? (endAddr - startAddr + 1) : ((endAddr - startAddr + 1) / 2);
    if (totalItems <= 0) {
        return "{ }";
    }

    std::ostringstream snippet;
    snippet << "{ ";
    int previewCount = std::min(totalItems, static_cast<int>(maxPreviewItems));
    for (int i = 0; i < previewCount; ++i) {
        if (i > 0) {
            snippet << ", ";
        }
        if (mode == 0) {
            auto addr = static_cast<Hardware::Word>(startAddr + i);
            auto val = static_cast<int8_t>(bus.ReadDirect(addr));
            snippet << static_cast<int>(val);
        } else {
            auto addr = static_cast<Hardware::Word>(startAddr + (i * 2));
            uint8_t low = bus.ReadDirect(addr);
            uint8_t high = bus.ReadDirect(static_cast<Hardware::Word>(addr + 1));
            auto val = static_cast<int16_t>(low | (high << 8));
            snippet << static_cast<int>(val);
        }
    }
    if (totalItems > previewCount) {
        snippet << ", ...";
    }
    snippet << " }";
    return snippet.str();
}

}  // namespace GUI
