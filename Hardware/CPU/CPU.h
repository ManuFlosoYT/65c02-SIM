#pragma once
#include <cstdint>
#include <iostream>
#include <iterator>

#include "Hardware/Core/Bus.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;
namespace CPUDispatch {
template <bool Debug>
inline int Dispatch(CPU& cpu, Bus& bus);
}

class CPU {
   public:
    // RETURNS:
    // 0: OK
    // 1: STOP/JAM
    // -1: Opcode no válido

    Word PC{};  // Program Counter
    Word SP{};  // Stack Pointer

    const Byte* current_page_ptr = nullptr;

    Byte A{}, X{}, Y{};  // Registros

    Byte C : 1;  // Carry Flag
    Byte Z : 1;  // Zero Flag
    Byte I : 1;  // Interrupt Disable
    Byte D : 1;  // Decimal Mode Flag
    Byte B : 1;  // Break Command
    Byte V : 1;  // Overflow Flag
    Byte N : 1;  // Negative Flag

    bool isInit = false;
    bool waiting = false;

    int remainingCycles = 0;
    bool cycleAccurate = true;

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inputStream);

    void Reset();

    int Execute(Bus& bus);

    template <bool Debug>
    int Step(Bus& bus);
    int Step(Bus& bus);

    template <bool Debug>
    void IRQ(Bus& bus);
    void IRQ(Bus& bus);

    template <bool Debug>
    int Dispatch(Bus& bus);

    [[nodiscard]] Byte GetStatus() const;
    void SetStatus(Byte status);

    [[nodiscard]] bool IsCycleAccurate() const;
    void SetCycleAccurate(bool enabled);
    [[nodiscard]] int GetRemainingCycles() const;

    void AddPageCrossPenalty(Word baseAddr, Word effectiveAddr);

    inline void UpdatePagePtr(Bus& bus);

    template <bool Debug>
    Byte FetchByte(Bus& bus);
    Byte FetchByte(Bus& bus);
    template <bool Debug>
    Word FetchWord(Bus& bus);
    Word FetchWord(Bus& bus);
    template <bool Debug>
    Byte ReadByte(Word addr, Bus& bus);
    Byte ReadByte(Word addr, Bus& bus);
    template <bool Debug>
    Word ReadWord(Word addr, Bus& bus);
    Word ReadWord(Word addr, Bus& bus);

    template <bool Debug>
    void PushByte(Byte val, Bus& bus);
    void PushByte(Byte val, Bus& bus);
    template <bool Debug>
    Byte PopByte(Bus& bus);
    Byte PopByte(Bus& bus);
    template <bool Debug>
    void PushWord(Word val, Bus& bus);
    void PushWord(Word val, Bus& bus);
    template <bool Debug>
    Word PopWord(Bus& bus);
    Word PopWord(Bus& bus);
};

}  // namespace Hardware

#include "Hardware/CPU/Dispatch.h"

inline void Hardware::CPU::Reset() {
    PC = 0xFFFC;
    SP = 0x01FF;

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
    isInit = false;
    waiting = false;
    remainingCycles = 0;
}

inline int Hardware::CPU::Execute(Bus& bus) {
    if (bus.HasActiveHooks()) {
        while (true) {
            int res = Step<true>(bus);
            if (res != 0) {
                return res;
            }
        }
    } else {
        while (true) {
            int res = Step<false>(bus);
            if (res != 0) {
                return res;
            }
        }
    }
    return 0;
}

template <bool Debug>
inline int Hardware::CPU::Step(Bus& bus) {
    if (!isInit) {
        PC = ReadWord<Debug>(0xFFFC, bus);
        UpdatePagePtr(bus);
        isInit = true;
    }

    if (waiting) {
        if ((bus.Read<Debug>(ACIA_STATUS) & 0x80) != 0) {
            waiting = false;
        } else {
            return 0;
        }
    }

    if (cycleAccurate && remainingCycles > 0) {
        remainingCycles--;
        return 0;
    }

    return Dispatch<Debug>(bus);
}

template <bool Debug>
inline void Hardware::CPU::IRQ(Bus& bus) {
    waiting = false;
    if (!I) {
        PushWord<Debug>(PC, bus);
        B = 0;
        PushByte<Debug>(GetStatus(), bus);
        I = 1;
        D = 0;
        PC = ReadWord<Debug>(0xFFFE, bus);
        UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline int Hardware::CPU::Dispatch(Bus& bus) {
    return CPUDispatch::Dispatch<Debug>(*this, bus);
}

inline Hardware::Byte Hardware::CPU::GetStatus() const {
    Byte status = 0;
    status |= C;
    status |= Z << 1;
    status |= I << 2;
    status |= D << 3;
    status |= B << 4;
    status |= V << 6;
    status |= N << 7;
    return status;
}

inline void Hardware::CPU::SetStatus(Byte status) {
    C = status & 0x01;
    Z = (status >> 1) & 0x01;
    I = (status >> 2) & 0x01;
    D = (status >> 3) & 0x01;
    B = (status >> 4) & 0x01;
    V = (status >> 6) & 0x01;
    N = (status >> 7) & 0x01;
}

inline bool Hardware::CPU::IsCycleAccurate() const { return cycleAccurate; }
inline void Hardware::CPU::SetCycleAccurate(bool enabled) { cycleAccurate = enabled; }
inline int Hardware::CPU::GetRemainingCycles() const { return remainingCycles; }

inline void Hardware::CPU::AddPageCrossPenalty(Word baseAddr, Word effectiveAddr) {
    if (cycleAccurate && ((baseAddr & 0xFF00) != (effectiveAddr & 0xFF00))) {
        remainingCycles++;
    }
}

inline void Hardware::CPU::UpdatePagePtr(Bus& bus) {
    Byte* memoryBase = bus.GetPageReadPtr(PC >> 8);
    if (memoryBase != nullptr) {
        current_page_ptr = std::next(memoryBase, PC);
    } else {
        current_page_ptr = nullptr;
    }
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::FetchByte(Bus& bus) {
    Byte dato = 0;
    if (current_page_ptr != nullptr) {
        dato = *current_page_ptr;
        current_page_ptr = std::next(current_page_ptr);
    } else {
        dato = bus.Read<Debug>(PC);
    }
    PC++;
    if ((PC & 0xFF) == 0) {
        UpdatePagePtr(bus);
    }
    return dato;
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::FetchWord(Bus& bus) {
    Word dato = FetchByte<Debug>(bus);
    dato |= (FetchByte<Debug>(bus) << 8);
    return dato;
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::ReadByte(Word addr, Bus& bus) {
    return bus.Read<Debug>(addr);
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::ReadWord(Word addr, Bus& bus) {
    Word dato = bus.Read<Debug>(addr);
    dato |= (bus.Read<Debug>(addr + 1) << 8);
    return dato;
}

template <bool Debug>
inline void Hardware::CPU::PushByte(Byte val, Bus& bus) {
    bus.Write<Debug>(SP, val);
    SP--;
    if (SP < 0x0100) {
        SP = 0x01FF;
    }
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::PopByte(Bus& bus) {
    SP++;
    if (SP > 0x01FF) {
        SP = 0x0100;
    }
    return bus.Read<Debug>(SP);
}

template <bool Debug>
inline void Hardware::CPU::PushWord(Word val, Bus& bus) {
    PushByte<Debug>((val >> 8) & 0xFF, bus);
    PushByte<Debug>(val & 0xFF, bus);
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::PopWord(Bus& bus) {
    Word Low = PopByte<Debug>(bus);
    Word High = PopByte<Debug>(bus);
    return (High << 8) | Low;
}

inline Hardware::Byte Hardware::CPU::FetchByte(Bus& bus) {
    if (bus.HasActiveHooks()) {
        return FetchByte<true>(bus);
    }
    return FetchByte<false>(bus);
}
inline Hardware::Word Hardware::CPU::FetchWord(Bus& bus) {
    if (bus.HasActiveHooks()) {
        return FetchWord<true>(bus);
    }
    return FetchWord<false>(bus);
}
inline Hardware::Byte Hardware::CPU::ReadByte(Word addr, Bus& bus) {
    if (bus.HasActiveHooks()) {
        return ReadByte<true>(addr, bus);
    }
    return ReadByte<false>(addr, bus);
}
inline Hardware::Word Hardware::CPU::ReadWord(Word addr, Bus& bus) {
    if (bus.HasActiveHooks()) {
        return ReadWord<true>(addr, bus);
    }
    return ReadWord<false>(addr, bus);
}
inline void Hardware::CPU::PushByte(Byte val, Bus& bus) {
    if (bus.HasActiveHooks()) {
        PushByte<true>(val, bus);
        return;
    }
    PushByte<false>(val, bus);
}
inline void Hardware::CPU::PushWord(Word val, Bus& bus) {
    if (bus.HasActiveHooks()) {
        PushWord<true>(val, bus);
        return;
    }
    PushWord<false>(val, bus);
}
inline Hardware::Byte Hardware::CPU::PopByte(Bus& bus) {
    if (bus.HasActiveHooks()) {
        return PopByte<true>(bus);
    }
    return PopByte<false>(bus);
}
inline Hardware::Word Hardware::CPU::PopWord(Bus& bus) {
    if (bus.HasActiveHooks()) {
        return PopWord<true>(bus);
    }
    return PopWord<false>(bus);
}

inline int Hardware::CPU::Step(Bus& bus) {
    if (bus.HasActiveHooks()) {
        return Step<true>(bus);
    }
    return Step<false>(bus);
}

inline void Hardware::CPU::IRQ(Bus& bus) {
    if (bus.HasActiveHooks()) {
        IRQ<true>(bus);
        return;
    }
    IRQ<false>(bus);
}
