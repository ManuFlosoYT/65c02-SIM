#include "RMB.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void RMB::Execute0(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11111110;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute1(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11111101;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute2(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11111011;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute3(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11110111;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute4(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11101111;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute5(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b11011111;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute6(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b10111111;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute7(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= 0b01111111;
    bus.Write<Debug>(ZP_Dir, data);
}

template void RMB::Execute0<true>(CPU&, Bus&);
template void RMB::Execute0<false>(CPU&, Bus&);
template void RMB::Execute1<true>(CPU&, Bus&);
template void RMB::Execute1<false>(CPU&, Bus&);
template void RMB::Execute2<true>(CPU&, Bus&);
template void RMB::Execute2<false>(CPU&, Bus&);
template void RMB::Execute3<true>(CPU&, Bus&);
template void RMB::Execute3<false>(CPU&, Bus&);
template void RMB::Execute4<true>(CPU&, Bus&);
template void RMB::Execute4<false>(CPU&, Bus&);
template void RMB::Execute5<true>(CPU&, Bus&);
template void RMB::Execute5<false>(CPU&, Bus&);
template void RMB::Execute6<true>(CPU&, Bus&);
template void RMB::Execute6<false>(CPU&, Bus&);
template void RMB::Execute7<true>(CPU&, Bus&);
template void RMB::Execute7<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
