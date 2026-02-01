#include "STY.h"

void STY::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem[ZP_Dir] = cpu.Y;
}

void STY::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    mem[ZP_Dir] = cpu.Y;
}

void STY::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem[Dir] = cpu.Y;
}