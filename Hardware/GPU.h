#pragma once

#include <iostream>

#include "Mem.h"

namespace Hardware {

class GPU {
public:
    // VRAM dimensions (visible area)
    const static Byte VRAM_WIDTH = 100;
    const static Byte VRAM_HEIGHT = 75;

    // Total display dimensions (including blanking)
    const static Byte DISPLAY_WIDTH = 132;
    const static Byte DISPLAY_HEIGHT = 78;

    // The CPU can only write to the first 64 rows of VRAM so we use this to
    // calculate if the GPU is in blanking interval
    const static Byte VRAM_HEIGHT_DRAWABLE_BY_CPU = 64;

    GPU() { Init(); }

    Byte vram[VRAM_HEIGHT][VRAM_WIDTH]{};

    void Init();

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    // Clock the GPU to advance pixel counters
    inline void Clock() {
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

    // Get current pixel position (14-bit values)
    Word GetPixelX() const { return pixelX; }

    Word GetPixelY() const { return pixelY; }

    // Bus control status
    bool IsInDrawingInterval() const {
        // We asume this is always true, since the CPU can only write to the
        // first 64 rows of VRAM The remaining 14 rows are only accessible by
        // the GPU istelf via loading a bin file into the GPU's memory.
        return pixelX < VRAM_WIDTH && pixelY < VRAM_HEIGHT_DRAWABLE_BY_CPU;
    }

    bool IsInBlankingInterval() const { return !IsInDrawingInterval(); }

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte operator[](Word addr) const {
        Byte x = addr & 0x7F;         // bits 0-6
        Byte y = (addr >> 7) & 0x7F;  // bits 7-13
        if (x >= VRAM_WIDTH || y >= VRAM_HEIGHT) return 0;
        return vram[y][x];
    }

    void SetWriteHook(Word address, WriteHook hook, void* context = nullptr);
    void Write(Word addr, Byte val);

    void SetReadHook(Word address, ReadHook hook, void* context = nullptr);
    Byte Read(Word addr);

private:
    WriteHook writeHooks[0x4000]{nullptr};
    void* writeContext[0x4000]{nullptr};
    bool hasWriteHook[0x4000]{false};

    ReadHook readHooks[0x4000]{nullptr};
    void* readContext[0x4000]{nullptr};
    bool hasReadHook[0x4000]{false};

    // Pixel counters (14-bit values)
    Word pixelX = 0;  // 0-131 (132 total)
    Word pixelY = 0;  // 0-77 (78 total)
};

}  // namespace Hardware
