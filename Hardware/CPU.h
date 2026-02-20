#pragma once
#include <cstdint>

#include "CPU/Instructions/InstructionSet.h"
#include "Mem.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU {
public:
    // RETURNS:
    // 0: OK
    // 1: STOP/JAM
    // -1: Opcode no válido

    Word PC{};  // Program Counter
    Word SP{};  // Stack Pointer

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

    CPU() = default;
    void Reset(Mem& mem);
    int Execute(Mem& mem);
    int Step(Mem& mem);
    void IRQ(Mem& mem);

    int Dispatch(Mem& mem);

    const Byte GetStatus() const;
    void SetStatus(Byte status);

    bool IsCycleAccurate() const { return cycleAccurate; }
    void SetCycleAccurate(bool enabled) { cycleAccurate = enabled; }
    int GetRemainingCycles() const { return remainingCycles; }

    // Helper to add penalty cycle for page boundary crossing
    inline void AddPageCrossPenalty(Word baseAddr, Word effectiveAddr) {
        if (cycleAccurate &&
            ((baseAddr & 0xFF00) != (effectiveAddr & 0xFF00))) {
            remainingCycles++;
        }
    }

    // Reads and advances PC
    inline const Byte FetchByte(const Mem& mem) {
        Byte dato = mem[PC];
        PC++;
        return dato;
    }

    inline const Word FetchWord(const Mem& mem) {
        Word dato = mem[PC];
        dato |= (mem[PC + 1] << 8);
        PC += 2;
        return dato;
    }

    // Reads without advancing PC
    inline const Byte ReadByte(const Word addr, Mem& mem) {
        return mem.Read(addr);
    }

    inline const Word ReadWord(const Word addr, Mem& mem) {
        Word dato = mem.Read(addr);
        dato |= (mem.Read(addr + 1) << 8);
        return dato;
    }

    Byte PopByte(Mem& mem);
    Word PopWord(Mem& mem);
    void PushByte(Byte val, Mem& mem);
    void PushWord(Word val, Mem& mem);
};

}  // namespace Hardware
