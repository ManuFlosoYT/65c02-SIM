#pragma once

#include <cstdint>

namespace Hardware {

struct RasterTimingGenerator {
    static constexpr uint32_t VRAM_WIDTH = 100;
    static constexpr uint32_t VRAM_HEIGHT = 75;
    static constexpr uint32_t DISPLAY_WIDTH = 132;
    static constexpr uint32_t DISPLAY_HEIGHT = 78;
    static constexpr uint32_t VRAM_HEIGHT_DRAWABLE_BY_CPU = 64;

    uint32_t pixelX = 0;
    uint32_t pixelY = 0;
    bool isBlanking = true;
    bool isYDrawing = true;
    uint32_t ticksToNextEvent = 0;

    void Reset() {
        pixelX = 0;
        pixelY = 0;
        isYDrawing = true;
        isBlanking = false;
        ticksToNextEvent = VRAM_WIDTH;
    }

    inline void Clock() {
        pixelX++;
        if (--ticksToNextEvent == 0) [[unlikely]] {
            if (pixelX == VRAM_WIDTH) {
                isBlanking = true;
                ticksToNextEvent = DISPLAY_WIDTH - VRAM_WIDTH;
            } else {
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

    [[nodiscard]] inline bool IsInBlankingInterval() const { return isBlanking; }

    [[nodiscard]] inline uint32_t GetDrawingWaitCycles() const {
        return (!isBlanking && ticksToNextEvent > 1) ? (ticksToNextEvent - 1) : 0;
    }

    inline void SkipDrawingCycles(uint32_t cycles) {
        pixelX += cycles;
        ticksToNextEvent -= cycles;
    }
};

}  // namespace Hardware
