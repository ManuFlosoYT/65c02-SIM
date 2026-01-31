#include "LDA.h"

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void LDA::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato;

    SetFlags(cpu);
}

void LDA::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.A = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.A = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    cpu.A = cpu.LeerByte(dir, mem);
    SetFlags(cpu);
}
