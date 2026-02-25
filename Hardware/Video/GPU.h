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
    void Clock();

    // Get current pixel position (14-bit values)
    Word GetPixelX() const;
    Word GetPixelY() const;

    // Bus control status
    bool IsInDrawingInterval() const;
    bool IsInBlankingInterval() const;

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte operator[](Word addr) const;

    void Write(Word addr, Byte val) override;
    Byte Read(Word addr) override;
    std::string GetName() const override;

private:
    // Pixel counters (14-bit values)
    Word pixelX = 0;  // 0-131 (132 total)
    Word pixelY = 0;  // 0-77 (78 total)
};

}  // namespace Hardware

#include "Hardware/Video/GPU.inl"
