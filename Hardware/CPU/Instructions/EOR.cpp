#include "EOR.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
void EOR::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato ^ cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteINDY(CPU& cpu, Bus& bus) {
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

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) ^ cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void EOR::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte<Debug>(dir, bus) ^ cpu.A;
    SetFlags(cpu);
}

template void EOR::ExecuteImmediate<true>(CPU&, Bus&);
template void EOR::ExecuteImmediate<false>(CPU&, Bus&);
template void EOR::ExecuteZP<true>(CPU&, Bus&);
template void EOR::ExecuteZP<false>(CPU&, Bus&);
template void EOR::ExecuteZPX<true>(CPU&, Bus&);
template void EOR::ExecuteZPX<false>(CPU&, Bus&);
template void EOR::ExecuteABS<true>(CPU&, Bus&);
template void EOR::ExecuteABS<false>(CPU&, Bus&);
template void EOR::ExecuteABSX<true>(CPU&, Bus&);
template void EOR::ExecuteABSX<false>(CPU&, Bus&);
template void EOR::ExecuteABSY<true>(CPU&, Bus&);
template void EOR::ExecuteABSY<false>(CPU&, Bus&);
template void EOR::ExecuteINDX<true>(CPU&, Bus&);
template void EOR::ExecuteINDX<false>(CPU&, Bus&);
template void EOR::ExecuteINDY<true>(CPU&, Bus&);
template void EOR::ExecuteINDY<false>(CPU&, Bus&);
template void EOR::ExecuteIND_ZP<true>(CPU&, Bus&);
template void EOR::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
