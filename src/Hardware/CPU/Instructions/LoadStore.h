#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class LDA {
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

class LDX {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu);
};

class LDY {
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
    static void SetFlags(CPU& cpu);
};

class STA {
public:
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
};

class STX {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
};

class STY {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
};

class STZ {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

inline void LDA::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void LDA::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato;

    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(dir, bus);
    SetFlags(cpu);
}

inline void LDX::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void LDX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.X = dato;

    SetFlags(cpu);
}

template <bool Debug>
inline void LDX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.X = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.Y;

    cpu.X = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.X = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDX::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.X = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

inline void LDY::SetFlags(CPU& cpu) {
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template <bool Debug>
inline void LDY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.Y = dato;

    SetFlags(cpu);
}

template <bool Debug>
inline void LDY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.Y = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDY::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.Y = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.Y = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void LDY::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.Y = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
inline void STA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.Y;
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    dir += cpu.Y;

    bus.Write<Debug>(dir, cpu.A);
}

template <bool Debug>
inline void STA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    bus.Write<Debug>(dir, cpu.A);
}

template <bool Debug>
inline void STX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.X);
}

template <bool Debug>
inline void STX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.Y;
    bus.Write<Debug>(ZP_Dir, cpu.X);
}

template <bool Debug>
inline void STX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.X);
}

template <bool Debug>
inline void STY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.Y);
}

template <bool Debug>
inline void STY::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, cpu.Y);
}

template <bool Debug>
inline void STY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.Y);
}

template <bool Debug>
inline void STZ::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, 0);
}

template <bool Debug>
inline void STZ::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, 0);
}

template <bool Debug>
inline void STZ::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, 0);
}

template <bool Debug>
inline void STZ::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    bus.Write<Debug>(Dir, 0);
}

}  // namespace Hardware::Instructions

