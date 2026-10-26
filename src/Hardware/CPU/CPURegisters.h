#pragma once

#include <cstdint>
#include <type_traits>

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

struct CPURegisters {
    Word PC{};
    Word SP{};
    Word instructionPC{0};

    Byte A{};
    Byte X{};
    Byte Y{};

    Byte C : 1 {0};
    Byte Z : 1 {0};
    Byte I : 1 {0};
    Byte D : 1 {0};
    Byte B : 1 {0};
    Byte V : 1 {0};
    Byte N : 1 {0};

    constexpr void Reset() noexcept {
        PC = 0xFFFC;
        SP = 0x01FF;
        instructionPC = 0;
        A = 0;
        X = 0;
        Y = 0;
        C = 0;
        Z = 0;
        I = 0;
        D = 0;
        B = 0;
        V = 0;
        N = 0;
    }

    [[nodiscard]] constexpr Byte GetStatus() const noexcept {
        Byte status = 0;
        status |= C;
        status |= Z << 1;
        status |= I << 2;
        status |= D << 3;
        status |= B << 4;
        status |= 1 << 5;
        status |= V << 6;
        status |= N << 7;
        return status;
    }

    constexpr void SetStatus(Byte status) noexcept {
        C = status & 0x01;
        Z = (status >> 1) & 0x01;
        I = (status >> 2) & 0x01;
        D = (status >> 3) & 0x01;
        B = (status >> 4) & 0x01;
        V = (status >> 6) & 0x01;
        N = (status >> 7) & 0x01;
    }
};

static_assert(std::is_standard_layout_v<CPURegisters>);
static_assert(std::is_trivially_copyable_v<CPURegisters>);

}  // namespace Hardware
