#pragma once

#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class AND {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu);
};

class ORA {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu);
};

class EOR {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu);
};

class BIT {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val);
};

class ASL {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val, Byte oldA);
};

class LSR {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val, Byte oldA);
};

class ROL {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val, Byte oldA);
};

class ROR {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val, Byte oldA);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

inline void AND::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void AND::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato & cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void AND::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(dir, bus) & cpu.A;
    SetFlags(cpu);
}

inline void ORA::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void ORA::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato | cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void ORA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(dir, bus) | cpu.A;
    SetFlags(cpu);
}

inline void EOR::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void EOR::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato ^ cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
inline void EOR::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

inline void BIT::SetFlags(CPU& cpu, Byte val) {
    cpu.N = (val & 0b10000000) > 0;
    cpu.V = (val & 0b01000000) > 0;
}

template <bool Debug>
inline void BIT::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
}

template <bool Debug>
inline void BIT::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void BIT::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void BIT::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void BIT::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

inline void ASL::SetFlags(CPU& cpu, Byte val, Byte oldA) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (oldA & 0b10000000) > 0;
}

template <bool Debug>
inline void ASL::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte oldA = cpu.A;
    cpu.A <<= 1;

    SetFlags(cpu, cpu.A, oldA);
}

template <bool Debug>
inline void ASL::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ASL::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ASL::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ASL::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

inline void LSR::SetFlags(CPU& cpu, Byte val, Byte oldA) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (oldA & 0b00000001) > 0;
}

template <bool Debug>
inline void LSR::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte oldA = cpu.A;
    cpu.A >>= 1;

    SetFlags(cpu, cpu.A, oldA);
}

template <bool Debug>
inline void LSR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato >>= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void LSR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato >>= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void LSR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato >>= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void LSR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato >>= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

inline void ROL::SetFlags(CPU& cpu, Byte val, Byte oldA) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (oldA & 0b10000000) > 0;
}

template <bool Debug>
inline void ROL::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte oldA = cpu.A;
    Byte oldCarry = cpu.C ? 1 : 0;
    cpu.A = (oldA << 1) | oldCarry;
    SetFlags(cpu, cpu.A, oldA);
}

template <bool Debug>
inline void ROL::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROL::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROL::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROL::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

inline void ROR::SetFlags(CPU& cpu, Byte val, Byte oldA) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (oldA & 0b00000001) > 0;
}

template <bool Debug>
inline void ROR::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte oldA = cpu.A;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    cpu.A = (oldA >> 1) | oldCarry;

    SetFlags(cpu, cpu.A, oldA);
}

template <bool Debug>
inline void ROR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
inline void ROR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

}  // namespace Hardware::Instructions

