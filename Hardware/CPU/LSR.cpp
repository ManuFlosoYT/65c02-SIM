#include "LSR.h"

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b00000001) > 0;
}

void LSR::ExecuteAccumulator(CPU& cpu, Mem& mem) {
    
    Byte A = cpu.A;
    cpu.A >>= 1;

    SetFlags(cpu, cpu.A, A);
}

void LSR::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte A = dato;
    dato >>= 1;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void LSR::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte A = dato;
    dato >>= 1;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void LSR::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte A = dato;
    dato >>= 1;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

void LSR::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte A = dato;
    dato >>= 1;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}