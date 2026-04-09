#include "ASL.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val, Byte oldA) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (oldA & 0b10000000) > 0;
}

template <bool Debug>
void ASL::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte oldA = cpu.A;
    cpu.A <<= 1;

    SetFlags(cpu, cpu.A, oldA);
}

template <bool Debug>
void ASL::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
void ASL::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
void ASL::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template <bool Debug>
void ASL::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte oldA = dato;
    dato <<= 1;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, oldA);
}

template void ASL::ExecuteAccumulator<true>(CPU&, Bus&);
template void ASL::ExecuteAccumulator<false>(CPU&, Bus&);
template void ASL::ExecuteZP<true>(CPU&, Bus&);
template void ASL::ExecuteZP<false>(CPU&, Bus&);
template void ASL::ExecuteZPX<true>(CPU&, Bus&);
template void ASL::ExecuteZPX<false>(CPU&, Bus&);
template void ASL::ExecuteABS<true>(CPU&, Bus&);
template void ASL::ExecuteABS<false>(CPU&, Bus&);
template void ASL::ExecuteABSX<true>(CPU&, Bus&);
template void ASL::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
