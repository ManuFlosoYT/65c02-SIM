#include "LDY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template <bool Debug>
void LDY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.Y = dato;

    SetFlags(cpu);
}

template <bool Debug>
void LDY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.Y = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDY::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.Y = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.Y = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDY::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.Y = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template void LDY::ExecuteImmediate<true>(CPU&, Bus&);
template void LDY::ExecuteImmediate<false>(CPU&, Bus&);
template void LDY::ExecuteZP<true>(CPU&, Bus&);
template void LDY::ExecuteZP<false>(CPU&, Bus&);
template void LDY::ExecuteZPX<true>(CPU&, Bus&);
template void LDY::ExecuteZPX<false>(CPU&, Bus&);
template void LDY::ExecuteABS<true>(CPU&, Bus&);
template void LDY::ExecuteABS<false>(CPU&, Bus&);
template void LDY::ExecuteABSX<true>(CPU&, Bus&);
template void LDY::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
