#include "ORA.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
void ORA::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    cpu.A = dato | cpu.A;

    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte<Debug>(ZP_Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    cpu.A = cpu.ReadByte<Debug>(Dir, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteINDY(CPU& cpu, Bus& bus) {
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

    cpu.A = cpu.ReadByte<Debug>(effectiveAddr, bus) | cpu.A;
    SetFlags(cpu);
}

template <bool Debug>
void ORA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = 0;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte<Debug>(dir, bus) | cpu.A;
    SetFlags(cpu);
}

template void ORA::ExecuteImmediate<true>(CPU&, Bus&);
template void ORA::ExecuteImmediate<false>(CPU&, Bus&);
template void ORA::ExecuteZP<true>(CPU&, Bus&);
template void ORA::ExecuteZP<false>(CPU&, Bus&);
template void ORA::ExecuteZPX<true>(CPU&, Bus&);
template void ORA::ExecuteZPX<false>(CPU&, Bus&);
template void ORA::ExecuteABS<true>(CPU&, Bus&);
template void ORA::ExecuteABS<false>(CPU&, Bus&);
template void ORA::ExecuteABSX<true>(CPU&, Bus&);
template void ORA::ExecuteABSX<false>(CPU&, Bus&);
template void ORA::ExecuteABSY<true>(CPU&, Bus&);
template void ORA::ExecuteABSY<false>(CPU&, Bus&);
template void ORA::ExecuteINDX<true>(CPU&, Bus&);
template void ORA::ExecuteINDX<false>(CPU&, Bus&);
template void ORA::ExecuteINDY<true>(CPU&, Bus&);
template void ORA::ExecuteINDY<false>(CPU&, Bus&);
template void ORA::ExecuteIND_ZP<true>(CPU&, Bus&);
template void ORA::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
