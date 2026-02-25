#include "Hardware/Video/GPU.h"

#include <cstring>

namespace Hardware {

void GPU::Reset() {
    std::memset(vram, 0, sizeof(vram));
    pixelX = 0;
    pixelY = 0;
}

// SetWriteHook removed.

void GPU::Write(Word addr, Byte val) {
    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;

    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        vram[y][x] = val;
    }
}

// SetReadHook removed.

Byte GPU::Read(Word addr) {
    // Addressing: A0-A6 = X, A7-A13 = Y
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        return vram[y][x];
    }
    return 0;
}

bool GPU::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&pixelX), sizeof(pixelX));
    out.write(reinterpret_cast<const char*>(&pixelY), sizeof(pixelY));
    out.write(reinterpret_cast<const char*>(vram), sizeof(vram));
    return out.good();
}

bool GPU::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&pixelX), sizeof(pixelX));
    in.read(reinterpret_cast<char*>(&pixelY), sizeof(pixelY));
    in.read(reinterpret_cast<char*>(vram), sizeof(vram));
    return in.good();
}

}  // namespace Hardware
