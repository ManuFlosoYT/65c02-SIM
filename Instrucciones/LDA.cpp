#include "LDA.h"

void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void LDA::EjecutarInmediato(CPU& cpu, Mem& mem) const {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato;

    SetFlags(cpu);
}

void LDA::EjecutarZP(CPU& cpu, Mem& mem) const {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.A = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarZPX(CPU& cpu, Mem& mem) const {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.A = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABS(CPU& cpu, Mem& mem) const {
    Byte Dir_Low = cpu.FetchByte(mem);
    Byte Dir_High = cpu.FetchByte(mem);

    Word Dir = (Dir_High << 8) | Dir_Low;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABSX(CPU& cpu, Mem& mem) const {
    Byte Dir_Low = cpu.FetchByte(mem);
    Byte Dir_High = cpu.FetchByte(mem);

    Word Dir = (Dir_High << 8) | Dir_Low;
    Dir += cpu.X;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarABSY(CPU& cpu, Mem& mem) const {
    Byte Dir_Low = cpu.FetchByte(mem);
    Byte Dir_High = cpu.FetchByte(mem);

    Word Dir = (Dir_High << 8) | Dir_Low;
    Dir += cpu.Y;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarINDX(CPU& cpu, Mem& mem) const {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Byte Dir_Low = cpu.LeerByte(ZP_Dir, mem);
    Byte Dir_High = cpu.LeerByte(ZP_Dir + 1, mem);

    Word Dir = (Dir_High << 8) | Dir_Low;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDA::EjecutarINDY(CPU& cpu, Mem& mem) const {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte Dir_Low = cpu.LeerByte(ZP_Dir, mem);
    Byte Dir_High = cpu.LeerByte(ZP_Dir + 1, mem);

    Word Dir = (Dir_High << 8) | Dir_Low;
    Dir += cpu.Y;

    cpu.A = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}
