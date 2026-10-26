#include "Hardware/Core/BusTelemetry.h"

namespace Hardware {

void BusTelemetry::DecayHeatmap(uint8_t amount) {
    if (!heatmapDirty.load(std::memory_order_relaxed)) {
        return;
    }
    bool anyRemaining = false;
    for (size_t i = 0; i < BUS_SIZE; ++i) {
        uint8_t r = readHeat.at(i).load(std::memory_order_relaxed);
        if (r > 0) {
            uint8_t nextR = (r > amount) ? static_cast<uint8_t>(r - amount) : 0;
            readHeat.at(i).store(nextR, std::memory_order_relaxed);
            if (nextR > 0) {
                anyRemaining = true;
            }
        }
        uint8_t w = writeHeat.at(i).load(std::memory_order_relaxed);
        if (w > 0) {
            uint8_t nextW = (w > amount) ? static_cast<uint8_t>(w - amount) : 0;
            writeHeat.at(i).store(nextW, std::memory_order_relaxed);
            if (nextW > 0) {
                anyRemaining = true;
            }
        }
    }
    if (!anyRemaining) {
        heatmapDirty.store(false, std::memory_order_relaxed);
    }
}

}  // namespace Hardware
