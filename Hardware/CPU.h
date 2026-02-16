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

    CPU() = default;
    void Reset(Mem& mem);
    int Execute(Mem& mem);
    int Step(Mem& mem);
    void IRQ(Mem& mem);

    int Dispatch(Mem& mem);

    const Byte GetStatus() const;
    void SetStatus(Byte status);

    const Byte FetchByte(const Mem& mem);  // Reads a byte from memory and advances PC
    const Byte ReadByte(const Word addr, Mem& mem);  // Reads a byte from memory without advancing PC
    const Word FetchWord(const Mem& mem);  // Reads a word from memory and advances PC + 2
    const Word ReadWord(const Word addr, Mem& mem);  // Reads a word from memory without advancing PC

    Byte PopByte(Mem& mem);
    Word PopWord(Mem& mem);
    void PushByte(Byte val, Mem& mem);
    void PushWord(Word val, Mem& mem);
};

}  // namespace Hardware
