#include "CPX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.X >= dato);
    cpu.Z = (cpu.X == dato);
    cpu.N = ((cpu.X - dato) & 0b10000000) > 0;
}

void CPX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);

    SetFlags(cpu, dato);
}

void CPX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

void CPX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    Byte dato = cpu.ReadByte(Dir, bus);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
