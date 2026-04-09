#include "DEC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

template <bool Debug>
void DEC::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    cpu.A--;
    SetFlags(cpu, cpu.A);
}

template <bool Debug>
void DEC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato--;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
void DEC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato--;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
void DEC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    dato--;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
void DEC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    dato--;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato);
}

template void DEC::ExecuteAccumulator<true>(CPU&, Bus&);
template void DEC::ExecuteAccumulator<false>(CPU&, Bus&);
template void DEC::ExecuteZP<true>(CPU&, Bus&);
template void DEC::ExecuteZP<false>(CPU&, Bus&);
template void DEC::ExecuteZPX<true>(CPU&, Bus&);
template void DEC::ExecuteZPX<false>(CPU&, Bus&);
template void DEC::ExecuteABS<true>(CPU&, Bus&);
template void DEC::ExecuteABS<false>(CPU&, Bus&);
template void DEC::ExecuteABSX<true>(CPU&, Bus&);
template void DEC::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
