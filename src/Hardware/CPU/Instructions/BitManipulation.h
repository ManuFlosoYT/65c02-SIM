#pragma once

#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class BBR {
public:
    template <bool Debug>
    static void Execute0(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute1(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute2(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute3(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute4(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute5(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute6(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute7(CPU& cpu, Bus& bus);

private:
    template <bool Debug, Byte Mask>
    static void ExecuteBBR(CPU& cpu, Bus& bus);
};

class BBS {
public:
    template <bool Debug>
    static void Execute0(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute1(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute2(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute3(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute4(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute5(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute6(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute7(CPU& cpu, Bus& bus);

private:
    template <bool Debug, Byte Mask>
    static void ExecuteBBS(CPU& cpu, Bus& bus);
};

class RMB {
public:
    template <bool Debug>
    static void Execute0(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute1(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute2(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute3(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute4(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute5(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute6(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute7(CPU& cpu, Bus& bus);

private:
    template <bool Debug, Byte Bit>
    static void ExecuteBit(CPU& cpu, Bus& bus);
};

class SMB {
public:
    template <bool Debug>
    static void Execute0(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute1(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute2(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute3(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute4(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute5(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute6(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void Execute7(CPU& cpu, Bus& bus);

private:
    template <bool Debug, Byte Bit>
    static void ExecuteBit(CPU& cpu, Bus& bus);
};

class TRB {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
};

class TSB {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug, Byte Mask>
inline void BBR::ExecuteBBR(CPU& cpu, Bus& bus) {
    Byte zpAddr = cpu.FetchByte<Debug>(bus);
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word target = static_cast<Word>(cpu.PC + offset);
    Byte val = cpu.ReadByte<Debug>(zpAddr, bus);

    if ((val & Mask) == 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, target);
        cpu.PC = target;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BBR::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00000001>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00000010>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00000100>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00001000>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00010000>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b00100000>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b01000000>(cpu, bus);
}

template <bool Debug>
inline void BBR::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBBR<Debug, 0b10000000>(cpu, bus);
}

template <bool Debug, Byte Mask>
inline void BBS::ExecuteBBS(CPU& cpu, Bus& bus) {
    Byte zpAddr = cpu.FetchByte<Debug>(bus);
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word target = static_cast<Word>(cpu.PC + offset);
    Byte val = cpu.ReadByte<Debug>(zpAddr, bus);

    if ((val & Mask) != 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, target);
        cpu.PC = target;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BBS::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000001>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000010>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000100>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00001000>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00010000>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00100000>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b01000000>(cpu, bus);
}

template <bool Debug>
inline void BBS::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b10000000>(cpu, bus);
}

template <bool Debug, Byte Bit>
inline void RMB::ExecuteBit(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= static_cast<Byte>(~(1U << Bit));
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
inline void RMB::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 0>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 1>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 2>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 3>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 4>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 5>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 6>(cpu, bus);
}

template <bool Debug>
inline void RMB::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 7>(cpu, bus);
}

template <bool Debug, Byte Bit>
inline void SMB::ExecuteBit(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= static_cast<Byte>(1U << Bit);
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
inline void SMB::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 0>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 1>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 2>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 3>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 4>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 5>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 6>(cpu, bus);
}

template <bool Debug>
inline void SMB::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 7>(cpu, bus);
}

template <bool Debug>
inline void TRB::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write<Debug>(ZP_Dir, res2);
}

template <bool Debug>
inline void TRB::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write<Debug>(Dir, res2);
}

template <bool Debug>
inline void TSB::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = cpu.A | dato;
    bus.Write<Debug>(ZP_Dir, res2);
}

template <bool Debug>
inline void TSB::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = cpu.A | dato;
    bus.Write<Debug>(Dir, res2);
}

}  // namespace Hardware::Instructions

