#include "AND.h"

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void AND::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato & cpu.A;

    SetFlags(cpu);
}

void AND::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.A = cpu.LeerByte(ZP_Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.A = cpu.LeerByte(ZP_Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.A = cpu.LeerByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    cpu.A = cpu.LeerByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    cpu.A = cpu.LeerByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    cpu.A = cpu.LeerByte(Dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarINDY(CPU& cpu, Mem& mem) {
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

    cpu.A = cpu.LeerByte(dir, mem) & cpu.A;
    SetFlags(cpu);
}

void AND::EjecutarIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.LeerByte(dir, mem) & cpu.A;
    SetFlags(cpu);
}
