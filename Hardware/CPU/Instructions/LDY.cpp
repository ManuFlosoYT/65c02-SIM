#include "LDY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

void LDY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);
    cpu.Y = dato;

    SetFlags(cpu);
}

void LDY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    cpu.Y = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDY::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;

    cpu.Y = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    cpu.Y = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

void LDY::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.Y = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
