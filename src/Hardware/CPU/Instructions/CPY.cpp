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

template <bool Debug>
void CPY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
void CPY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CPY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template void CPY::ExecuteImmediate<true>(CPU&, Bus&);
template void CPY::ExecuteImmediate<false>(CPU&, Bus&);
template void CPY::ExecuteZP<true>(CPU&, Bus&);
template void CPY::ExecuteZP<false>(CPU&, Bus&);
template void CPY::ExecuteABS<true>(CPU&, Bus&);
template void CPY::ExecuteABS<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
