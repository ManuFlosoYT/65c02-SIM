#include "Hardware/Video/GPU.h"

namespace Hardware {

void GPU::Reset() {
    rasterTiming.Reset();
    for (auto& row : vram) {
        row.fill(0);
    }
    vramDirty.store(true, std::memory_order_relaxed);
}

void GPU::LoadVRAM(std::span<const uint8_t> data) {
    for (size_t yIndex = 0; yIndex < VRAM_HEIGHT; yIndex++) {
        for (size_t xIndex = 0; xIndex < VRAM_WIDTH; xIndex++) {
            size_t addr = (yIndex * 128) + xIndex;
            if (addr < data.size()) {
                vram[yIndex][xIndex] = data[addr];
            }
        }
    }
    vramDirty.store(true, std::memory_order_relaxed);
}

bool GPU::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, vram);
    ISerializable::Serialize(out, rasterTiming.pixelX);
    ISerializable::Serialize(out, rasterTiming.pixelY);
    ISerializable::Serialize(out, rasterTiming.isBlanking);
    ISerializable::Serialize(out, rasterTiming.isYDrawing);
    ISerializable::Serialize(out, rasterTiming.ticksToNextEvent);
    return out.good();
}

bool GPU::LoadState(std::istream& inputStream) {
    ISerializable::Deserialize(inputStream, vram);
    ISerializable::Deserialize(inputStream, rasterTiming.pixelX);
    ISerializable::Deserialize(inputStream, rasterTiming.pixelY);
    ISerializable::Deserialize(inputStream, rasterTiming.isBlanking);
    ISerializable::Deserialize(inputStream, rasterTiming.isYDrawing);
    ISerializable::Deserialize(inputStream, rasterTiming.ticksToNextEvent);
    vramDirty.store(true, std::memory_order_relaxed);
    return inputStream.good();
}

}  // namespace Hardware
