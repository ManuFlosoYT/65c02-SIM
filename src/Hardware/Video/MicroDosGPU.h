#pragma once

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>

#include "Hardware/Video/GPU.h"

namespace Hardware {

class MicroDosGPU : public GPU {
   public:
    static constexpr size_t MEMORY_SIZE = 0x2000;

    MicroDosGPU() = default;

    [[nodiscard]] bool BlocksCPU() const override { return false; }
    [[nodiscard]] std::string GetName() const override { return "microDOS-GPU"; }

    void Reset() override;
    void LoadVRAM(std::span<const uint8_t> data) override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    inline void Write(Word addr, Byte val) override;
    inline void WriteDirect(Word addr, Byte val) override;
    inline Byte Read(Word addr) override;
    inline Byte ReadDirect(Word addr) override;

    [[nodiscard]] const std::array<Byte, MEMORY_SIZE>& GetExtraRAM() const { return extraRam; }
    [[nodiscard]] std::array<Byte, MEMORY_SIZE>& GetExtraRAM() { return extraRam; }

   private:
    std::array<Byte, MEMORY_SIZE> extraRam{};
};

inline void MicroDosGPU::Write(Word addr, Byte val) {
    Byte pixelXAddr = addr & 0x7F;
    Byte pixelYAddr = (addr >> 7) & 0x7F;
    if (pixelXAddr < VRAM_WIDTH && pixelYAddr < VRAM_HEIGHT) {
        GetVRAM()[pixelYAddr][pixelXAddr] = val;
        SetVRAMDirty();
    } else if (addr < MEMORY_SIZE) {
        extraRam[addr] = val;
    }
}

inline void MicroDosGPU::WriteDirect(Word addr, Byte val) {
    Write(addr, val);
}

inline Byte MicroDosGPU::Read(Word addr) {
    Byte pixelXAddr = addr & 0x7F;
    Byte pixelYAddr = (addr >> 7) & 0x7F;
    if (pixelXAddr < VRAM_WIDTH && pixelYAddr < VRAM_HEIGHT) {
        return GetVRAM()[pixelYAddr][pixelXAddr];
    }
    return (addr < MEMORY_SIZE) ? extraRam[addr] : 0;
}

inline Byte MicroDosGPU::ReadDirect(Word addr) {
    return Read(addr);
}

}  // namespace Hardware

