#pragma once
#include <cstdint>
#include <iostream>

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
    bool LoadState(std::istream& in);

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

    const Byte GetStatus() const;
    void SetStatus(Byte status);

    bool IsCycleAccurate() const;
    void SetCycleAccurate(bool enabled);
    int GetRemainingCycles() const;

    void AddPageCrossPenalty(Word baseAddr, Word effectiveAddr);

    inline void UpdatePagePtr(Bus& bus);

    template <bool Debug>
    const Byte FetchByte(Bus& bus);
    const Byte FetchByte(Bus& bus);
    template <bool Debug>
    const Word FetchWord(Bus& bus);
    const Word FetchWord(Bus& bus);
    template <bool Debug>
    const Byte ReadByte(const Word addr, Bus& bus);
    const Byte ReadByte(const Word addr, Bus& bus);
    template <bool Debug>
    const Word ReadWord(const Word addr, Bus& bus);
    const Word ReadWord(const Word addr, Bus& bus);

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

#include "Hardware/CPU/CPU.inl"
