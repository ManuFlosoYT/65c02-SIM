#pragma once
#include <cstdint>

#include "CPU/Instructions/InstructionSet.h"
#include "Mem.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;
namespace CPUDispatch {
int Dispatch(CPU& cpu, Mem& mem);
}

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

    inline void Reset(Mem& mem) {
        PC = 0xFFFC;
        SP = 0x01FF;  // Top of Stack

        // Reset registers
        A = 0;
        X = 0;
        Y = 0;

        // Reset flags
        C = 0;
        Z = 0;
        I = 0;
        D = 0;
        B = 0;
        V = 0;
        N = 0;
        isInit = false;
    }

    inline int Execute(Mem& mem) {
        while (true) {
            int res = Step(mem);
            if (res != 0) return res;
        }
        return 0;
    }

    inline int Step(Mem& mem) {
        if (!isInit) {
            PC = ReadWord(0xFFFC, mem);
            isInit = true;
        }

        // Check interrupts
        if (waiting) {
            if ((mem.memory[ACIA_STATUS] & 0x80) != 0) {
                waiting = false;
            } else {
                return 0;
            }
        }

        // In cycle-accurate mode, consume remaining cycles
        if (cycleAccurate && remainingCycles > 0) {
            remainingCycles--;
            return 0;
        }

        return Dispatch(mem);
    }

    inline void IRQ(Mem& mem) {
        waiting = false;
        if (!I) {
            PushWord(PC, mem);
            B = 0;
            PushByte(GetStatus(), mem);
            I = 1;
            D = 0;  // 65C02 clears Decimal flag in IRQ
            PC = ReadWord(0xFFFE, mem);
        }
    }

    inline int Dispatch(Mem& mem) { return CPUDispatch::Dispatch(*this, mem); }

    inline const Byte GetStatus() const {
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

    inline void SetStatus(Byte status) {
        C = status & 0x01;
        Z = (status >> 1) & 0x01;
        I = (status >> 2) & 0x01;
        D = (status >> 3) & 0x01;
        B = (status >> 4) & 0x01;
        V = (status >> 6) & 0x01;
        N = (status >> 7) & 0x01;
    }

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

    inline void PushByte(Byte val, Mem& mem) {
        mem.Write(SP, val);
        SP--;
        if (SP < 0x0100) SP = 0x01FF;  // Wrap to the top of the stack
    }

    inline Byte PopByte(Mem& mem) {
        SP++;
        if (SP > 0x01FF) SP = 0x0100;  // Wrap to the bottom of the stack
        return mem.Read(SP);
    }

    inline void PushWord(Word val, Mem& mem) {
        PushByte((val >> 8) & 0xFF, mem);
        PushByte(val & 0xFF, mem);
    }

    inline Word PopWord(Mem& mem) {
        Word Low = PopByte(mem);
        Word High = PopByte(mem);
        return (High << 8) | Low;
    }
};

}  // namespace Hardware
