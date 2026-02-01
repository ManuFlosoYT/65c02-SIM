#include "BIT.h"

static void SetFlags(CPU& cpu, Byte val) {
    cpu.N = (val & 0b10000000) > 0;
    cpu.V = (val & 0b01000000) > 0;
}

void BIT::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
}

void BIT::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.LeerByte(Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.LeerByte(Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}