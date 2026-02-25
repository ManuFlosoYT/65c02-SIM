// GPU.inl — Inline implementations for Hardware::GPU

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
