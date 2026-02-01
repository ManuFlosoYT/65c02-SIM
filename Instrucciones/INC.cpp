#include "INC.h"

static void SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

void INC::EjecutarAcumulador(CPU& cpu, Mem& mem) {
    cpu.A++;
    SetFlags(cpu, cpu.A);
}

void INC::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    dato++;
    mem[ZP_Dir] = dato;
    SetFlags(cpu, dato);
}

void INC::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    dato++;
    mem[ZP_Dir] = dato;
    SetFlags(cpu, dato);
}

void INC::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.LeerByte(Dir, mem);
    dato++;
    mem[Dir] = dato;
    SetFlags(cpu, dato);
}

void INC::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.LeerByte(Dir, mem);
    dato++;
    mem[Dir] = dato;
    SetFlags(cpu, dato);
}