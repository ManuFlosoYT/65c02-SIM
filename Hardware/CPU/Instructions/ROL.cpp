#include "ROL.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b10000000) > 0;
}

template <bool Debug>
void ROL::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte A = cpu.A;
    Byte oldCarry = cpu.C ? 1 : 0;
    cpu.A = (A << 1) | oldCarry;
    SetFlags(cpu, cpu.A, A);
}

template <bool Debug>
void ROL::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void ROL::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void ROL::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, A);
}

template <bool Debug>
void ROL::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 1 : 0;
    dato = (dato << 1) | oldCarry;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato, A);
}

template void ROL::ExecuteAccumulator<true>(CPU&, Bus&);
template void ROL::ExecuteAccumulator<false>(CPU&, Bus&);
template void ROL::ExecuteZP<true>(CPU&, Bus&);
template void ROL::ExecuteZP<false>(CPU&, Bus&);
template void ROL::ExecuteZPX<true>(CPU&, Bus&);
template void ROL::ExecuteZPX<false>(CPU&, Bus&);
template void ROL::ExecuteABS<true>(CPU&, Bus&);
template void ROL::ExecuteABS<false>(CPU&, Bus&);
template void ROL::ExecuteABSX<true>(CPU&, Bus&);
template void ROL::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
