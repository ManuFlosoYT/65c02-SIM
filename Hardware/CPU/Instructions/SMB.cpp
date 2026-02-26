#include "SMB.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void SMB::Execute0(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00000001;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute1(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00000010;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute2(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00000100;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute3(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00001000;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute4(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00010000;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute5(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b00100000;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute6(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b01000000;
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void SMB::Execute7(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data |= 0b10000000;
    bus.Write<Debug>(ZP_Dir, data);
}

template void SMB::Execute0<true>(CPU&, Bus&);
template void SMB::Execute0<false>(CPU&, Bus&);
template void SMB::Execute1<true>(CPU&, Bus&);
template void SMB::Execute1<false>(CPU&, Bus&);
template void SMB::Execute2<true>(CPU&, Bus&);
template void SMB::Execute2<false>(CPU&, Bus&);
template void SMB::Execute3<true>(CPU&, Bus&);
template void SMB::Execute3<false>(CPU&, Bus&);
template void SMB::Execute4<true>(CPU&, Bus&);
template void SMB::Execute4<false>(CPU&, Bus&);
template void SMB::Execute5<true>(CPU&, Bus&);
template void SMB::Execute5<false>(CPU&, Bus&);
template void SMB::Execute6<true>(CPU&, Bus&);
template void SMB::Execute6<false>(CPU&, Bus&);
template void SMB::Execute7<true>(CPU&, Bus&);
template void SMB::Execute7<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
