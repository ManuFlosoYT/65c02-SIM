#include "CPX.h"

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.X >= dato);
    cpu.Z = (cpu.X == dato);
    cpu.N = ((cpu.X - dato) & 0b10000000) > 0;
}

void CPX::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CPX::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CPX::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}