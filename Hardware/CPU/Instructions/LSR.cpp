#include "LSR.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b00000001) > 0;
}

template <bool Debug>
void LSR::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    
    Byte A = cpu.A;
    cpu.A >>= 1;

    SetFlags(cpu, cpu.A, A);
}

template <bool Debug>
void LSR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte A = dato;
    dato >>= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void LSR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte A = dato;
    dato >>= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void LSR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte A = dato;
    dato >>= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void LSR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte A = dato;
    dato >>= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, A);
}

template void LSR::ExecuteAccumulator<true>(CPU&, Bus&);
template void LSR::ExecuteAccumulator<false>(CPU&, Bus&);
template void LSR::ExecuteZP<true>(CPU&, Bus&);
template void LSR::ExecuteZP<false>(CPU&, Bus&);
template void LSR::ExecuteZPX<true>(CPU&, Bus&);
template void LSR::ExecuteZPX<false>(CPU&, Bus&);
template void LSR::ExecuteABS<true>(CPU&, Bus&);
template void LSR::ExecuteABS<false>(CPU&, Bus&);
template void LSR::ExecuteABSX<true>(CPU&, Bus&);
template void LSR::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
