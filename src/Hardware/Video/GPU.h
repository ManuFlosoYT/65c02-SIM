#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <span>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class GPU : public IBusDevice {
   public:
    // VRAM dimensions (visible area)
    static constexpr Byte VRAM_WIDTH = 100;
    static constexpr Byte VRAM_HEIGHT = 75;

    // Total display dimensions (including blanking)
    static constexpr Byte DISPLAY_WIDTH = 132;
    static constexpr Byte DISPLAY_HEIGHT = 78;

    // The CPU can only write to the first 64 rows of VRAM so we use this to
    // calculate if the GPU is in blanking interval
    static constexpr Byte VRAM_HEIGHT_DRAWABLE_BY_CPU = 64;

    GPU();
    void Reset() override;
    void LoadVRAM(std::span<const uint8_t> data);

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    // Clock the GPU to advance pixel counters
    inline void Clock();

    // Bus control status
    [[nodiscard]] inline Word GetPixelX() const { return static_cast<Word>(pixelX); }
    [[nodiscard]] inline Word GetPixelY() const { return static_cast<Word>(pixelY); }
    [[nodiscard]] inline bool IsInBlankingInterval() const;
    [[nodiscard]] uint32_t GetDrawingWaitCycles() const {
        return (!isBlanking && ticksToNextEvent > 1) ? (ticksToNextEvent - 1) : 0;
    }
    void SkipDrawingCycles(uint32_t cycles) {
        pixelX += cycles;
        ticksToNextEvent -= cycles;
    }

    [[nodiscard]] bool ConsumeVRAMDirty() const { return vramDirty.exchange(false, std::memory_order_relaxed); }

    inline void Write(Word addr, Byte val) override;
    inline void WriteDirect(Word addr, Byte val) override;
    inline Byte Read(Word addr) override;
    inline Byte ReadDirect(Word addr) override;
    [[nodiscard]] inline std::string GetName() const override;

    [[nodiscard]] const std::array<std::array<Byte, VRAM_WIDTH>, VRAM_HEIGHT>& GetVRAM() const { return vram; }
    [[nodiscard]] std::array<std::array<Byte, VRAM_WIDTH>, VRAM_HEIGHT>& GetVRAM() { return vram; }

   private:
    std::array<std::array<Byte, VRAM_WIDTH>, VRAM_HEIGHT> vram{};
    mutable std::atomic<bool> vramDirty{true};

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

inline bool GPU::IsInBlankingInterval() const { return isBlanking; }

inline void GPU::Write(Word addr, Byte val) {
    Byte pixelXAddr = addr & 0x7F;
    Byte pixelYAddr = (addr >> 7) & 0x7F;
    if (pixelXAddr < VRAM_WIDTH && pixelYAddr < VRAM_HEIGHT) {
        vram[pixelYAddr][pixelXAddr] = val;
        vramDirty.store(true, std::memory_order_relaxed);
    }
}

inline void GPU::WriteDirect(Word addr, Byte val) {
    Byte pixelXAddr = addr & 0x7F;
    Byte pixelYAddr = (addr >> 7) & 0x7F;
    if (pixelXAddr < VRAM_WIDTH && pixelYAddr < VRAM_HEIGHT) {
        vram[pixelYAddr][pixelXAddr] = val;
        vramDirty.store(true, std::memory_order_relaxed);
    }
}

inline Byte GPU::Read(Word addr) {
    Byte pixelXAddr = addr & 0x7F;
    Byte pixelYAddr = (addr >> 7) & 0x7F;
    if (pixelXAddr < VRAM_WIDTH && pixelYAddr < VRAM_HEIGHT) {
        return vram[pixelYAddr][pixelXAddr];
    }
    return 0;
}

inline Byte GPU::ReadDirect(Word addr) {
    return Read(addr);
}

inline std::string GPU::GetName() const { return "GPU"; }

}  // namespace Hardware
