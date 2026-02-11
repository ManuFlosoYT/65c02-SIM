#include "LDX.h"

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

void LDX::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.X = dato;

    SetFlags(cpu);
}

void LDX::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.X = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDX::EjecutarZPY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.Y;

    cpu.X = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDX::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.X = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDX::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    cpu.X = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}
