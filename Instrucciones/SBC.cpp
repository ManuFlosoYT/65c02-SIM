#include "SBC.h"

static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = !(res > 0xFF);
    cpu.V = ((oldA ^ res) & (oldA ^ dato) & 0b10000000) > 0;
}

void SBC::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

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
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}

void SBC::EjecutarIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.LeerByte(dir, mem);
    Word res = cpu.A - dato - (1 - cpu.C);
    cpu.A = res;

    SetFlags(cpu, res, dato, oldA);
}