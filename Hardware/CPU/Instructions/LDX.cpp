#include "LDX.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

void LDX::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    cpu.X = dato;

    SetFlags(cpu);
}

void LDX::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    cpu.X = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDX::ExecuteZPY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.Y;

    cpu.X = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu);
}

void LDX::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    cpu.X = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

void LDX::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.X = cpu.ReadByte(Dir, mem);
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
