#include "CMP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.A >= dato);
    cpu.Z = (cpu.A == dato);
    cpu.N = ((cpu.A - dato) & 0b10000000) > 0;
}

template <bool Debug>
void CMP::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
void CMP::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte<Debug>(dir, bus);
    SetFlags(cpu, dato);
}

template void CMP::ExecuteImmediate<true>(CPU&, Bus&);
template void CMP::ExecuteImmediate<false>(CPU&, Bus&);
template void CMP::ExecuteZP<true>(CPU&, Bus&);
template void CMP::ExecuteZP<false>(CPU&, Bus&);
template void CMP::ExecuteZPX<true>(CPU&, Bus&);
template void CMP::ExecuteZPX<false>(CPU&, Bus&);
template void CMP::ExecuteABS<true>(CPU&, Bus&);
template void CMP::ExecuteABS<false>(CPU&, Bus&);
template void CMP::ExecuteABSX<true>(CPU&, Bus&);
template void CMP::ExecuteABSX<false>(CPU&, Bus&);
template void CMP::ExecuteABSY<true>(CPU&, Bus&);
template void CMP::ExecuteABSY<false>(CPU&, Bus&);
template void CMP::ExecuteINDX<true>(CPU&, Bus&);
template void CMP::ExecuteINDX<false>(CPU&, Bus&);
template void CMP::ExecuteINDY<true>(CPU&, Bus&);
template void CMP::ExecuteINDY<false>(CPU&, Bus&);
template void CMP::ExecuteIND_ZP<true>(CPU&, Bus&);
template void CMP::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
