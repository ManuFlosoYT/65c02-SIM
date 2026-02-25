#include "Hardware/Video/GPU.h"

#include <cstring>

namespace Hardware {

void GPU::Reset() {
    pixelX = 0;
    pixelY = 0;
    isYDrawing = true;
    isBlanking = false;
    ticksToNextEvent = VRAM_WIDTH;
    std::memset(vram, 0, sizeof(vram));
}

bool GPU::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&vram), sizeof(vram));
    Word px = pixelX;
    Word py = pixelY;
    out.write(reinterpret_cast<const char*>(&px), sizeof(px));
    out.write(reinterpret_cast<const char*>(&py), sizeof(py));
    return out.good();
}

bool GPU::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&vram), sizeof(vram));

    Word px, py;
    in.read(reinterpret_cast<char*>(&px), sizeof(px));
    in.read(reinterpret_cast<char*>(&py), sizeof(py));
    pixelX = px;
    pixelY = py;

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

    return in.good();
}

}  // namespace Hardware
