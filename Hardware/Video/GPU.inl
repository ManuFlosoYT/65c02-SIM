// GPU.inl — Inline implementations for Hardware::GPU

inline Hardware::GPU::GPU() { Reset(); }

inline void Hardware::GPU::Clock() {
    pixelX++;
    if (pixelX >= DISPLAY_WIDTH) {
        pixelX = 0;
        pixelY++;
        if (pixelY >= DISPLAY_HEIGHT) {
            pixelY = 0;
        }
    }
}

inline Hardware::Word Hardware::GPU::GetPixelX() const { return pixelX; }
inline Hardware::Word Hardware::GPU::GetPixelY() const { return pixelY; }

inline bool Hardware::GPU::IsInDrawingInterval() const {
    // We asume this is always true, since the CPU can only write to the
    // first 64 rows of VRAM The remaining 14 rows are only accessible by
    // the GPU istelf via loading a bin file into the GPU's memory.
    return pixelX < VRAM_WIDTH && pixelY < VRAM_HEIGHT_DRAWABLE_BY_CPU;
}

inline bool Hardware::GPU::IsInBlankingInterval() const {
    return !IsInDrawingInterval();
}

inline Hardware::Byte Hardware::GPU::operator[](Word addr) const {
    Byte x = addr & 0x7F;         // bits 0-6
    Byte y = (addr >> 7) & 0x7F;  // bits 7-13
    if (x >= VRAM_WIDTH || y >= VRAM_HEIGHT) return 0;
    return vram[y][x];
}

inline std::string Hardware::GPU::GetName() const { return "GPU"; }
