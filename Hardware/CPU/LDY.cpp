#include "LDY.h"

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

void LDY::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.Y = dato;

    SetFlags(cpu);
}

void LDY::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.Y = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDY::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.Y = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDY::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.Y = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}

void LDY::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    cpu.Y = cpu.LeerByte(Dir, mem);
    SetFlags(cpu);
}
