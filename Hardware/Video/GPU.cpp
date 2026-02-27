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

bool GPU::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(vram.data()), sizeof(vram));  // NOLINT
    Word currPixelX = pixelX;
    Word currPixelY = pixelY;
    out.write(reinterpret_cast<const char*>(&currPixelX), sizeof(currPixelX));  // NOLINT
    out.write(reinterpret_cast<const char*>(&currPixelY), sizeof(currPixelY));  // NOLINT
    return out.good();
}

bool GPU::LoadState(std::istream& inputStream) {
    inputStream.read(reinterpret_cast<char*>(vram.data()), sizeof(vram));  // NOLINT

    Word currPixelX = 0;
    Word currPixelY = 0;
    inputStream.read(reinterpret_cast<char*>(&currPixelX), sizeof(currPixelX));  // NOLINT
    inputStream.read(reinterpret_cast<char*>(&currPixelY), sizeof(currPixelY));  // NOLINT
    pixelX = currPixelX;
    pixelY = currPixelY;

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
