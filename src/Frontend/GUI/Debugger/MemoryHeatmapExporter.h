#pragma once

#include "Frontend/Control/AppState.h"
#include <span>
#include <string>

namespace GUI {

class MemoryHeatmapExporter {
public:
    static void UpdateTexture(Control::AppState& state,
                              std::span<const uint32_t> readCounts,
                              std::span<const uint32_t> writeCounts,
                              std::span<unsigned char> pixels);

    static std::string GenerateRawData(std::span<const uint32_t> readCounts,
                                       std::span<const uint32_t> writeCounts);

    static void DrawControls(Control::AppState& state,
                             std::span<const uint32_t> readCounts,
                             std::span<const uint32_t> writeCounts,
                             std::span<unsigned char> pixels,
                             bool& profilerDirty);

#ifndef TARGET_WASM
    static void HandleDialogs(Control::AppState& state,
                              std::span<const uint32_t> readCounts,
                              std::span<const uint32_t> writeCounts,
                              std::span<unsigned char> pixels);
#endif
};

}  // namespace GUI
