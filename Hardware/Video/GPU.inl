// GPU.inl — Inline implementations for Hardware::GPU

namespace Hardware {

inline GPU::GPU() { Reset(); }

inline void GPU::Clock() {
    pixelX++;
    if (pixelX >= DISPLAY_WIDTH) {
        pixelX = 0;
        pixelY++;
        if (pixelY >= DISPLAY_HEIGHT) {
            pixelY = 0;
        }
        // State update
        isYDrawing = (pixelY < VRAM_HEIGHT_DRAWABLE_BY_CPU);
        isBlanking = !isYDrawing;
    } else if (pixelX == VRAM_WIDTH) {
        isBlanking = true;
    }
}

inline Word GPU::GetPixelX() const { return pixelX; }
inline Word GPU::GetPixelY() const { return pixelY; }

inline bool GPU::IsInDrawingInterval() const { return !isBlanking; }

inline bool GPU::IsInBlankingInterval() const { return isBlanking; }

inline Byte GPU::operator[](Word addr) const {
    Byte x = addr & 0x7F;         // bits 0-6
    Byte y = (addr >> 7) & 0x7F;  // bits 7-13
    if (x >= VRAM_WIDTH || y >= VRAM_HEIGHT) return 0;
    return vram[y][x];
}

inline std::string GPU::GetName() const { return "GPU"; }

}  // namespace Hardware
