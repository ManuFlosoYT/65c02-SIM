#include "STX.h"

void STX::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem.Write(ZP_Dir, cpu.X);
}

void STX::ExecuteZPY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.Y;
    mem.Write(ZP_Dir, cpu.X);
}

void STX::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem.Write(Dir, cpu.X);
}