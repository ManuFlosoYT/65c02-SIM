#include "STX.h"

void STX::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem[ZP_Dir] = cpu.X;
}

void STX::EjecutarZPY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.Y;
    mem[ZP_Dir] = cpu.X;
}

void STX::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem[Dir] = cpu.X;
}