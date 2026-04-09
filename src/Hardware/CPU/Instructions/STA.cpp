#include "STA.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void STA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.Y;
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    bus.Write<Debug>(Dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = 0;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    bus.Write<Debug>(dir, cpu.A);
}

template <bool Debug>
void STA::ExecuteINDZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = 0;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    bus.Write<Debug>(dir, cpu.A);
}

template void STA::ExecuteZP<true>(CPU&, Bus&);
template void STA::ExecuteZP<false>(CPU&, Bus&);
template void STA::ExecuteZPX<true>(CPU&, Bus&);
template void STA::ExecuteZPX<false>(CPU&, Bus&);
template void STA::ExecuteABS<true>(CPU&, Bus&);
template void STA::ExecuteABS<false>(CPU&, Bus&);
template void STA::ExecuteABSX<true>(CPU&, Bus&);
template void STA::ExecuteABSX<false>(CPU&, Bus&);
template void STA::ExecuteABSY<true>(CPU&, Bus&);
template void STA::ExecuteABSY<false>(CPU&, Bus&);
template void STA::ExecuteINDX<true>(CPU&, Bus&);
template void STA::ExecuteINDX<false>(CPU&, Bus&);
template void STA::ExecuteINDY<true>(CPU&, Bus&);
template void STA::ExecuteINDY<false>(CPU&, Bus&);
template void STA::ExecuteINDZP<true>(CPU&, Bus&);
template void STA::ExecuteINDZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
