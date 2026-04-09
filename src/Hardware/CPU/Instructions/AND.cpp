#include "AND.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
void AND::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato & cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteINDY(CPU& cpu, Bus& bus) {
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

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) & cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void AND::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = 0;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte<Debug>(dir, bus) & cpu.A;
    SetFlags(cpu);
}

template void AND::ExecuteImmediate<true>(CPU&, Bus&);
template void AND::ExecuteImmediate<false>(CPU&, Bus&);
template void AND::ExecuteZP<true>(CPU&, Bus&);
template void AND::ExecuteZP<false>(CPU&, Bus&);
template void AND::ExecuteZPX<true>(CPU&, Bus&);
template void AND::ExecuteZPX<false>(CPU&, Bus&);
template void AND::ExecuteABS<true>(CPU&, Bus&);
template void AND::ExecuteABS<false>(CPU&, Bus&);
template void AND::ExecuteABSX<true>(CPU&, Bus&);
template void AND::ExecuteABSX<false>(CPU&, Bus&);
template void AND::ExecuteABSY<true>(CPU&, Bus&);
template void AND::ExecuteABSY<false>(CPU&, Bus&);
template void AND::ExecuteINDX<true>(CPU&, Bus&);
template void AND::ExecuteINDX<false>(CPU&, Bus&);
template void AND::ExecuteINDY<true>(CPU&, Bus&);
template void AND::ExecuteINDY<false>(CPU&, Bus&);
template void AND::ExecuteIND_ZP<true>(CPU&, Bus&);
template void AND::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
