#include "CPX.h"

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.X >= dato);
    cpu.Z = (cpu.X == dato);
    cpu.N = ((cpu.X - dato) & 0b10000000) > 0;
}

void CPX::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CPX::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CPX::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}