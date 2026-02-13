#include "LDA.h"

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void LDA::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato;

    SetFlags(cpu);
}

void LDA::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.A = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.A = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    cpu.A = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    cpu.A = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord(ZP_Dir, mem);

    cpu.A = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    cpu.A = cpu.ReadByte(dir, mem);
    SetFlags(cpu);
}

void LDA::ExecuteIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte(dir, mem);
    SetFlags(cpu);
}
