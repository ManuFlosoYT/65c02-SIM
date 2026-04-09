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

template <bool Debug>
void CPX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
void CPX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CPX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template void CPX::ExecuteImmediate<true>(CPU&, Bus&);
template void CPX::ExecuteImmediate<false>(CPU&, Bus&);
template void CPX::ExecuteZP<true>(CPU&, Bus&);
template void CPX::ExecuteZP<false>(CPU&, Bus&);
template void CPX::ExecuteABS<true>(CPU&, Bus&);
template void CPX::ExecuteABS<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
