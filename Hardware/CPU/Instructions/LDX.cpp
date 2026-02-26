#include "LDX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
void LDX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.X = dato;

    SetFlags(cpu);
}

template <bool Debug>
void LDX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.X = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.Y;

    cpu.X = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.X = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDX::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.X = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template void LDX::ExecuteImmediate<true>(CPU&, Bus&);
template void LDX::ExecuteImmediate<false>(CPU&, Bus&);
template void LDX::ExecuteZP<true>(CPU&, Bus&);
template void LDX::ExecuteZP<false>(CPU&, Bus&);
template void LDX::ExecuteZPY<true>(CPU&, Bus&);
template void LDX::ExecuteZPY<false>(CPU&, Bus&);
template void LDX::ExecuteABS<true>(CPU&, Bus&);
template void LDX::ExecuteABS<false>(CPU&, Bus&);
template void LDX::ExecuteABSY<true>(CPU&, Bus&);
template void LDX::ExecuteABSY<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
