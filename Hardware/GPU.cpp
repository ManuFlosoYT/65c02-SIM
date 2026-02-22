#include "GPU.h"

#include <cstring>

namespace Hardware {

void GPU::Init() {
    std::memset(vram, 0, sizeof(vram));
    for (Word i = 0; i < 0x4000; i++) {
        hasWriteHook[i] = false;
        hasReadHook[i] = false;
    }
    pixelX = 0;
    pixelY = 0;
}

void GPU::SetWriteHook(Word address, WriteHook hook, void* context) {
    if (address < 0x4000) {
        writeHooks[address] = hook;
        writeContext[address] = context;
        hasWriteHook[address] = true;
    }
}

void GPU::Write(Word addr, Byte val) {
    // Check hooks first
    if (addr < 0x4000 && hasWriteHook[addr]) {
        writeHooks[addr](writeContext[addr], addr, val);
    }

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;

    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        vram[y][x] = val;
    }
}

void GPU::SetReadHook(Word address, ReadHook hook, void* context) {
    if (address < 0x4000) {
        readHooks[address] = hook;
        readContext[address] = context;
        hasReadHook[address] = true;
    }
}

Byte GPU::Read(Word addr) {
    if (addr < 0x4000 && hasReadHook[addr]) {
        return readHooks[addr](readContext[addr], addr);
    }

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
    return out.good();
}

bool GPU::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&pixelX), sizeof(pixelX));
    in.read(reinterpret_cast<char*>(&pixelY), sizeof(pixelY));
    return in.good();
}

}  // namespace Hardware
