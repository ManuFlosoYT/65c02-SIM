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
    }
}

inline Word GPU::GetPixelX() const { return pixelX; }
inline Word GPU::GetPixelY() const { return pixelY; }

inline bool GPU::IsInDrawingInterval() const {
    // We asume this is always true, since the CPU can only write to the
    // first 64 rows of VRAM The remaining 14 rows are only accessible by
    // the GPU istelf via loading a bin file into the GPU's memory.
    return pixelX < VRAM_WIDTH && pixelY < VRAM_HEIGHT_DRAWABLE_BY_CPU;
}

inline bool GPU::IsInBlankingInterval() const { return !IsInDrawingInterval(); }

inline Byte GPU::operator[](Word addr) const {
    Byte x = addr & 0x7F;         // bits 0-6
    Byte y = (addr >> 7) & 0x7F;  // bits 7-13
    if (x >= VRAM_WIDTH || y >= VRAM_HEIGHT) return 0;
    return vram[y][x];
}

inline std::string GPU::GetName() const { return "GPU"; }

}  // namespace Hardware
