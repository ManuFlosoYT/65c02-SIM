#include "CMP.h"

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.A >= dato);
    cpu.Z = (cpu.A == dato);
    cpu.N = ((cpu.A - dato) & 0b10000000) > 0;
}

void CMP::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CMP::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarINDY(CPU& cpu, Mem& mem) {
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

    Byte dato = cpu.LeerByte(dir, mem);
    SetFlags(cpu, dato);
}

void CMP::EjecutarIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.LeerByte(dir, mem);
    SetFlags(cpu, dato);
}   