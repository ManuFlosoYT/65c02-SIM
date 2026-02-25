#include "CMP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.A >= dato);
    cpu.Z = (cpu.A == dato);
    cpu.N = ((cpu.A - dato) & 0b10000000) > 0;
}

void CMP::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);

    SetFlags(cpu, dato);
}

void CMP::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord(ZP_Dir, bus);

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte(effectiveAddr, bus);
    SetFlags(cpu, dato);
}

void CMP::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte(dir, bus);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
