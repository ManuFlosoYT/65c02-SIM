#pragma once

#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class GPU : public IBusDevice {
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

    GPU() { Reset(); }
    void Reset() override;

    Byte vram[VRAM_HEIGHT][VRAM_WIDTH]{};

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

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

    void Write(Word addr, Byte val) override;
    Byte Read(Word addr) override;
    std::string GetName() const override { return "GPU"; }

private:
    // Pixel counters (14-bit values)
    Word pixelX = 0;  // 0-131 (132 total)
    Word pixelY = 0;  // 0-77 (78 total)
};

}  // namespace Hardware
