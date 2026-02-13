#ifndef SIM_65C02_GPU_H
#define SIM_65C02_GPU_H

#include <cstdint>
#include <functional>
#include <map>

using Byte = uint8_t;
using Word = uint16_t;

using WriteHook = std::function<void(Word, Byte)>;
using ReadHook = std::function<Byte(Word)>;

class GPU {
public:
    // VRAM dimensions (visible area)
    const static Byte VRAM_WIDTH = 100;
    const static Byte VRAM_HEIGHT = 75;

    // Total display dimensions (including blanking)
    const static Byte DISPLAY_WIDTH = 132;
    const static Byte DISPLAY_HEIGHT = 78;

    GPU() { Init(); }
    Byte vram[VRAM_HEIGHT][VRAM_WIDTH]{};

    void Init();

    // Clock the GPU to advance pixel counters
    void Clock();

    // Get current pixel position (14-bit values)
    Word GetPixelX() const { return pixelX; }
    Word GetPixelY() const { return pixelY; }

    // Bus control status
    bool IsInDrawingInterval() const {
        return pixelX < VRAM_WIDTH && pixelY < VRAM_HEIGHT;
    }
    bool IsInBlankingInterval() const { return !IsInDrawingInterval(); }

    // Addressing: A0-A6 = X (0-99), A7-A13 = Y (0-74)
    Byte operator[](Word addr) const {
        Byte x = addr & 0x7F;         // bits 0-6
        Byte y = (addr >> 7) & 0x7F;  // bits 7-13
        if (x >= VRAM_WIDTH || y >= VRAM_HEIGHT) return 0;
        return vram[y][x];
    }

    void SetWriteHook(Word address, WriteHook hook);
    void Write(Word addr, Byte val);

    void SetReadHook(Word address, ReadHook hook);
    Byte Read(Word addr);

private:
    std::map<Word, WriteHook> writeHooks;
    std::map<Word, ReadHook> readHooks;

    // Pixel counters (14-bit values)
    Word pixelX = 0;  // 0-131 (132 total)
    Word pixelY = 0;  // 0-77 (78 total)
};

#endif  // SIM_65C02_GPU_H