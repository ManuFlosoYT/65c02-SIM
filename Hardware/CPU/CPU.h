#pragma once
#include <cstdint>
#include <iostream>

#include "Hardware/Core/Bus.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;
namespace CPUDispatch {
int Dispatch(CPU& cpu, Bus& bus);
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

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    inline void Reset() {
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

    inline int Execute(Bus& bus) {
        while (true) {
            int res = Step(bus);
            if (res != 0) return res;
        }
        return 0;
    }

    inline int Step(Bus& bus) {
        if (!isInit) {
            PC = ReadWord(0xFFFC, bus);
            isInit = true;
        }

        if (waiting) {
            if ((bus.Read(ACIA_STATUS) & 0x80) != 0) {
                waiting = false;
            } else {
                return 0;
            }
        }

        if (cycleAccurate && remainingCycles > 0) {
            remainingCycles--;
            return 0;
        }

        return Dispatch(bus);
    }

    inline void IRQ(Bus& bus) {
        waiting = false;
        if (!I) {
            PushWord(PC, bus);
            B = 0;
            PushByte(GetStatus(), bus);
            I = 1;
            D = 0;
            PC = ReadWord(0xFFFE, bus);
        }
    }

    inline int Dispatch(Bus& bus) { return CPUDispatch::Dispatch(*this, bus); }

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

    inline void AddPageCrossPenalty(Word baseAddr, Word effectiveAddr) {
        if (cycleAccurate &&
            ((baseAddr & 0xFF00) != (effectiveAddr & 0xFF00))) {
            remainingCycles++;
        }
    }

    inline const Byte FetchByte(Bus& bus) {
        Byte dato = bus.Read(PC);
        PC++;
        return dato;
    }

    inline const Word FetchWord(Bus& bus) {
        Word dato = bus.Read(PC);
        dato |= (bus.Read(PC + 1) << 8);
        PC += 2;
        return dato;
    }

    inline const Byte ReadByte(const Word addr, Bus& bus) {
        return bus.Read(addr);
    }

    inline const Word ReadWord(const Word addr, Bus& bus) {
        Word dato = bus.Read(addr);
        dato |= (bus.Read(addr + 1) << 8);
        return dato;
    }

    inline void PushByte(Byte val, Bus& bus) {
        bus.Write(SP, val);
        SP--;
        if (SP < 0x0100) SP = 0x01FF;
    }

    inline Byte PopByte(Bus& bus) {
        SP++;
        if (SP > 0x01FF) SP = 0x0100;
        return bus.Read(SP);
    }

    inline void PushWord(Word val, Bus& bus) {
        PushByte((val >> 8) & 0xFF, bus);
        PushByte(val & 0xFF, bus);
    }

    inline Word PopWord(Bus& bus) {
        Word Low = PopByte(bus);
        Word High = PopByte(bus);
        return (High << 8) | Low;
    }
};

}  // namespace Hardware
