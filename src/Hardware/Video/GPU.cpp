#include "Hardware/Video/GPU.h"

namespace Hardware {

void GPU::Reset() {
    pixelX = 0;
    pixelY = 0;
    isYDrawing = true;
    isBlanking = false;
    ticksToNextEvent = VRAM_WIDTH;
    for (auto& row : vram) {
        row.fill(0);
    }
}

void GPU::LoadVRAM(std::span<const uint8_t> data) {
    for (size_t yIndex = 0; yIndex < VRAM_HEIGHT; yIndex++) {
        for (size_t xIndex = 0; xIndex < VRAM_WIDTH; xIndex++) {
            size_t addr = (yIndex * 128) + xIndex;
            if (addr < data.size()) {
                vram.at(yIndex).at(xIndex) = data[addr];
            }
        }
    }
}

bool GPU::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, vram);
    ISerializable::Serialize(out, pixelX);
    ISerializable::Serialize(out, pixelY);
    return out.good();
}

bool GPU::LoadState(std::istream& inputStream) {
    ISerializable::Deserialize(inputStream, vram);
    ISerializable::Deserialize(inputStream, pixelX);
    ISerializable::Deserialize(inputStream, pixelY);

    if (pixelX >= DISPLAY_WIDTH) {
        pixelX %= DISPLAY_WIDTH;
    }
    if (pixelY >= DISPLAY_HEIGHT) {
        pixelY %= DISPLAY_HEIGHT;
    }

    isYDrawing = (pixelY < VRAM_HEIGHT_DRAWABLE_BY_CPU);

    if (!isYDrawing) {
        isBlanking = true;
        ticksToNextEvent = DISPLAY_WIDTH - pixelX;
    } else {
        if (pixelX < VRAM_WIDTH) {
            isBlanking = false;
            ticksToNextEvent = VRAM_WIDTH - pixelX;
        } else {
            isBlanking = true;
            ticksToNextEvent = DISPLAY_WIDTH - pixelX;
        }
    }

    return inputStream.good();
}

}  // namespace Hardware
