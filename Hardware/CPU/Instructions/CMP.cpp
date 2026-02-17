#include "CMP.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.A >= dato);
    cpu.Z = (cpu.A == dato);
    cpu.N = ((cpu.A - dato) & 0b10000000) > 0;
}

void CMP::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CMP::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord(ZP_Dir, mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte(effectiveAddr, mem);
    SetFlags(cpu, dato);
}

void CMP::ExecuteIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte(dir, mem);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
