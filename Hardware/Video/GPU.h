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

    GPU();
    void Reset() override;

    Byte vram[VRAM_HEIGHT][VRAM_WIDTH]{};

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    // Clock the GPU to advance pixel counters
    inline void Clock();

    // Get current pixel position (14-bit values)
    inline Word GetPixelX() const;
    inline Word GetPixelY() const;

    // Bus control status
    inline bool IsInDrawingInterval() const;
    inline bool IsInBlankingInterval() const;

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    inline Byte operator[](Word addr) const;

    inline void Write(Word addr, Byte val) override;
    inline Byte Read(Word addr) override;
    inline std::string GetName() const override;

private:
    // Pixel counters (32-bit native to prevent partial register stalls on x86)
    uint32_t pixelX = 0;  // 0-131 (132 total)
    uint32_t pixelY = 0;  // 0-77 (78 total)

    // Cached states for O(1) interval access without arithmetic/branches
    bool isBlanking = true;
    bool isYDrawing = true;
    uint32_t ticksToNextEvent = 0;
};

}  // namespace Hardware


namespace Hardware {

inline GPU::GPU() { Reset(); }

inline void GPU::Clock() {
    pixelX++;
    if (--ticksToNextEvent == 0) [[unlikely]] {
        if (pixelX == VRAM_WIDTH) {  // Finished drawing area of scanline
            isBlanking = true;
            ticksToNextEvent = DISPLAY_WIDTH - VRAM_WIDTH;
        } else {  // Finished scanline completely
            pixelX = 0;

            if (++pixelY == VRAM_HEIGHT_DRAWABLE_BY_CPU) {
                isYDrawing = false;
            } else if (pixelY == DISPLAY_HEIGHT) {
                pixelY = 0;
                isYDrawing = true;
            }

            if (isYDrawing) {
                isBlanking = false;
                ticksToNextEvent = VRAM_WIDTH;
            } else {
                isBlanking = true;
                ticksToNextEvent = DISPLAY_WIDTH;
            }
        }
    }
}

inline Word GPU::GetPixelX() const { return pixelX; }
inline Word GPU::GetPixelY() const { return pixelY; }

inline bool GPU::IsInDrawingInterval() const { return !isBlanking; }

inline bool GPU::IsInBlankingInterval() const { return isBlanking; }

inline Byte GPU::operator[](Word addr) const {
    Byte x = addr & 0x7F;         // bits 0-6
    Byte y = (addr >> 7) & 0x7F;  // bits 7-13
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        return vram[y][x];
    }
    return 0;
}

inline void GPU::Write(Word addr, Byte val) {
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        vram[y][x] = val;
    }
}

inline Byte GPU::Read(Word addr) {
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        return vram[y][x];
    }
    return 0;
}

inline std::string GPU::GetName() const { return "GPU"; }

}  // namespace Hardware
