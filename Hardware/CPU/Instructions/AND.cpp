#include "AND.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void AND::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato & cpu.A;

    SetFlags(cpu);
}

void AND::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.A = cpu.ReadByte(ZP_Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte(ZP_Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.A = cpu.ReadByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord(ZP_Dir, mem);

    cpu.A = cpu.ReadByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteINDY(CPU& cpu, Mem& mem) {
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

    cpu.A = cpu.ReadByte(effectiveAddr, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::ExecuteIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte(dir, mem) & cpu.A;
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
