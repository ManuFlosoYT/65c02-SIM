#include "CPY.h"

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.Y >= dato);
    cpu.Z = (cpu.Y == dato);
    cpu.N = ((cpu.Y - dato) & 0b10000000) > 0;
}

void CPY::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CPY::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CPY::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    SetFlags(cpu, dato);
}