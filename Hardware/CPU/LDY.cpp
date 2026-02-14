#include "LDY.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

void LDY::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.Y = dato;

    SetFlags(cpu);
}

void LDY::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.Y = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDY::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    cpu.Y = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDY::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.Y = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDY::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    cpu.Y = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
