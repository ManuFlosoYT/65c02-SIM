#include "BBR.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BBR::Execute0(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000001) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute1(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000010) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute2(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000100) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute3(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00001000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute4(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00010000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute5(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00100000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute6(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b01000000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBR::Execute7(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b10000000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BBR::Execute0<true>(CPU&, Bus&);
template void BBR::Execute0<false>(CPU&, Bus&);
template void BBR::Execute1<true>(CPU&, Bus&);
template void BBR::Execute1<false>(CPU&, Bus&);
template void BBR::Execute2<true>(CPU&, Bus&);
template void BBR::Execute2<false>(CPU&, Bus&);
template void BBR::Execute3<true>(CPU&, Bus&);
template void BBR::Execute3<false>(CPU&, Bus&);
template void BBR::Execute4<true>(CPU&, Bus&);
template void BBR::Execute4<false>(CPU&, Bus&);
template void BBR::Execute5<true>(CPU&, Bus&);
template void BBR::Execute5<false>(CPU&, Bus&);
template void BBR::Execute6<true>(CPU&, Bus&);
template void BBR::Execute6<false>(CPU&, Bus&);
template void BBR::Execute7<true>(CPU&, Bus&);
template void BBR::Execute7<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
