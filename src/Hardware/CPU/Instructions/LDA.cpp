#include "LDA.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
void LDA::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato;

    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = 0;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu);
}

template <bool Debug>
void LDA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = 0;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte<Debug>(dir, bus);
    SetFlags(cpu);
}

template void LDA::ExecuteImmediate<true>(CPU&, Bus&);
template void LDA::ExecuteImmediate<false>(CPU&, Bus&);
template void LDA::ExecuteZP<true>(CPU&, Bus&);
template void LDA::ExecuteZP<false>(CPU&, Bus&);
template void LDA::ExecuteZPX<true>(CPU&, Bus&);
template void LDA::ExecuteZPX<false>(CPU&, Bus&);
template void LDA::ExecuteABS<true>(CPU&, Bus&);
template void LDA::ExecuteABS<false>(CPU&, Bus&);
template void LDA::ExecuteABSX<true>(CPU&, Bus&);
template void LDA::ExecuteABSX<false>(CPU&, Bus&);
template void LDA::ExecuteABSY<true>(CPU&, Bus&);
template void LDA::ExecuteABSY<false>(CPU&, Bus&);
template void LDA::ExecuteINDX<true>(CPU&, Bus&);
template void LDA::ExecuteINDX<false>(CPU&, Bus&);
template void LDA::ExecuteINDY<true>(CPU&, Bus&);
template void LDA::ExecuteINDY<false>(CPU&, Bus&);
template void LDA::ExecuteIND_ZP<true>(CPU&, Bus&);
template void LDA::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
