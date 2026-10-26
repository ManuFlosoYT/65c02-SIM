#pragma once

#include <array>
#include <cstdint>

namespace Hardware {

using Byte = uint8_t;

struct ACIAFrameConfig {
    static constexpr std::array<int, 16> BaudRates = {
        0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
    };

    static constexpr int CalculateBaudRate(Byte ctrl) noexcept {
        return BaudRates[ctrl & 0x0F];
    }

    static constexpr int CalculateFrameBits(Byte ctrl, Byte cmd) noexcept {
        int bits = 1;  // Start bit

        // Word length
        Byte wl = (ctrl >> 5) & 0x03;
        switch (wl) {
            case 0: bits += 8; break;
            case 1: bits += 7; break;
            case 2: bits += 6; break;
            case 3: bits += 5; break;
            default: break;
        }

        // Parity
        if ((cmd & 0x20) != 0) {
            bits += 1;
        }

        // Stop bits
        if ((ctrl & 0x80) != 0) {
            bits += 2;
        } else {
            bits += 1;
        }

        return bits;
    }
};

constexpr int CalculateBaudRate(Byte ctrl) noexcept {
    return ACIAFrameConfig::CalculateBaudRate(ctrl);
}

constexpr int CalculateFrameBits(Byte ctrl, Byte cmd) noexcept {
    return ACIAFrameConfig::CalculateFrameBits(ctrl, cmd);
}

static_assert(ACIAFrameConfig::CalculateBaudRate(0) == 0);
static_assert(ACIAFrameConfig::CalculateBaudRate(0x0F) == 19200);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x00, 0x00) == 10);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x20, 0x00) == 9);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x40, 0x00) == 8);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x60, 0x00) == 7);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x00, 0x20) == 11);
static_assert(ACIAFrameConfig::CalculateFrameBits(0x80, 0x00) == 11);
static_assert(ACIAFrameConfig::CalculateFrameBits(0xA0, 0x20) == 11);

}  // namespace Hardware
