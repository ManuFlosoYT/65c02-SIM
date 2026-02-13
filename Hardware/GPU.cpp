#include "GPU.h"

#include <cstring>

void GPU::Init() {
    std::memset(vram, 0, sizeof(vram));
    writeHooks.clear();
    readHooks.clear();
    pixelX = 0;
    pixelY = 0;
}

void GPU::Clock() {
    // Increment pixel counters
    pixelX++;
    if (pixelX >= DISPLAY_WIDTH) {
        pixelX = 0;
        pixelY++;
        if (pixelY >= DISPLAY_HEIGHT) {
            pixelY = 0;
        }
    }
}

void GPU::SetWriteHook(Word address, WriteHook hook) {
    writeHooks[address] = hook;
}

void GPU::Write(Word addr, Byte val) {
    // Check hooks first
    auto it = writeHooks.find(addr);
    if (it != writeHooks.end()) {
        it->second(addr, val);
    }

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;

    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        vram[y][x] = val;
    }
}

void GPU::SetReadHook(Word address, ReadHook hook) {
    readHooks[address] = hook;
}

Byte GPU::Read(Word addr) {
    auto it = readHooks.find(addr);
    if (it != readHooks.end()) {
        return it->second(addr);
    }

    // Addressing: A0-A6 = X, A7-A13 = Y
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        return vram[y][x];
    }
    return 0;
}
