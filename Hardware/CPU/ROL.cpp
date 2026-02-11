#include "ROL.h"

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b10000000) > 0;
}

void ROL::EjecutarAcumulador(CPU& cpu, Mem& mem) {
    Byte A = cpu.A;
    Byte oldCarry = cpu.C ? 1 : 0;
    cpu.A = (A << 1) | oldCarry;
    SetFlags(cpu, cpu.A, A);
}

void ROL::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROL::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROL::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.LeerByte(Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROL::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.LeerByte(Dir, mem);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}