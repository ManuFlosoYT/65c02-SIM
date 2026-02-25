#include "CPY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.Y >= dato);
    cpu.Z = (cpu.Y == dato);
    cpu.N = ((cpu.Y - dato) & 0b10000000) > 0;
}

void CPY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);

    SetFlags(cpu, dato);
}

void CPY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

void CPY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
