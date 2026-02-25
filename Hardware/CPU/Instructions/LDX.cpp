#include "LDX.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

void LDX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);
    cpu.X = dato;

    SetFlags(cpu);
}

void LDX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    cpu.X = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.Y;

    cpu.X = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    cpu.X = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

void LDX::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.X = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
