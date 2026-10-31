#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>

#include "Hardware/Video/GPU.h"
#include "Hardware/Comm/PS2MouseController.h"

namespace Hardware {

class MicroDosGPU : public GPU {
   public:
    static constexpr size_t MEMORY_SIZE = 0x2000;
    static constexpr Word MOUSE_BTNS_OFFSET = 0x1FF0U;
    static constexpr Word MOUSE_X_OFFSET = 0x1FF1U;
    static constexpr Word MOUSE_Y_OFFSET = 0x1FF2U;

    static constexpr Byte MOUSE_BTN_LEFT = 0x01U;
    static constexpr Byte MOUSE_BTN_RIGHT = 0x02U;
    static constexpr Byte MOUSE_BTN_MIDDLE = 0x04U;

    MicroDosGPU() = default;

    [[nodiscard]] bool BlocksCPU() const override { return false; }
    [[nodiscard]] std::string GetName() const override { return "microDOS-GPU"; }

    void Reset() override;
    void Clock() override;
    void LoadVRAM(std::span<const uint8_t> data) override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    inline void Write(Word addr, Byte val) override;
    inline void WriteDirect(Word addr, Byte val) override;
    inline Byte Read(Word addr) override;
    inline Byte ReadDirect(Word addr) override;

    void UpdateHostMouse(Byte targetX, Byte targetY, Byte buttons) noexcept;

    [[nodiscard]] const PS2MouseController& GetMouse() const noexcept { return mouse; }
    [[nodiscard]] PS2MouseController& GetMouse() noexcept { return mouse; }
    [[nodiscard]] uint8_t GetBurstStep() const noexcept { return burstStep; }
    [[nodiscard]] Word GetFpgaAddr() const noexcept { return fpgaAddr; }
    [[nodiscard]] Byte GetFpgaData() const noexcept { return fpgaData; }
    [[nodiscard]] bool GetFpgaRw() const noexcept { return fpgaRw; }

    [[nodiscard]] const std::array<Byte, MEMORY_SIZE>& GetExtraRAM() const { return extraRam; }
    [[nodiscard]] std::array<Byte, MEMORY_SIZE>& GetExtraRAM() { return extraRam; }

   private:
    std::array<Byte, MEMORY_SIZE> extraRam{};
    PS2MouseController mouse{};
    uint8_t burstStep = 0U;
    Word fpgaAddr = 0U;
    Byte fpgaData = 0U;
    bool fpgaRw = true;
    std::atomic<Byte> hostTargetX{0U};
    std::atomic<Byte> hostTargetY{0U};
    std::atomic<Byte> hostTargetButtons{0U};
    std::atomic<bool> hostMousePending{false};
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

