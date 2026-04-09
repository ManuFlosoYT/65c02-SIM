#include "BIT.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.N = (val & 0b10000000) > 0;
    cpu.V = (val & 0b01000000) > 0;
}

template <bool Debug>
void BIT::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
}

template <bool Debug>
void BIT::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
void BIT::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
void BIT::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template <bool Debug>
void BIT::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

template void BIT::ExecuteImmediate<true>(CPU&, Bus&);
template void BIT::ExecuteImmediate<false>(CPU&, Bus&);
template void BIT::ExecuteZP<true>(CPU&, Bus&);
template void BIT::ExecuteZP<false>(CPU&, Bus&);
template void BIT::ExecuteZPX<true>(CPU&, Bus&);
template void BIT::ExecuteZPX<false>(CPU&, Bus&);
template void BIT::ExecuteABS<true>(CPU&, Bus&);
template void BIT::ExecuteABS<false>(CPU&, Bus&);
template void BIT::ExecuteABSX<true>(CPU&, Bus&);
template void BIT::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
