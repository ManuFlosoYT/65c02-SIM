#include "ROR.h"

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b00000001) > 0;
}

void ROR::ExecuteAccumulator(CPU& cpu, Mem& mem) {
    Byte A = cpu.A;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    cpu.A = (A >> 1) | oldCarry;

    SetFlags(cpu, cpu.A, A);
}

void ROR::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}