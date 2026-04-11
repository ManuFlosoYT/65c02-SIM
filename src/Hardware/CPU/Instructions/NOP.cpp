#include "NOP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void NOP::Execute(CPU& cpu, Bus& bus) {
    // Bruh, you really thought NOP would do something lmao 😂😂😂😂
}

template <bool Debug>
void NOP::ExecuteImmediate(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
void NOP::ExecuteZP(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
void NOP::ExecuteZPX(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
void NOP::ExecuteABS(CPU& cpu, Bus& bus) {
    cpu.FetchWord<Debug>(bus);
}

template <bool Debug>
void NOP::ExecuteABSX(CPU& cpu, Bus& bus) {
    cpu.FetchWord<Debug>(bus);
}

template void NOP::Execute<true>(CPU&, Bus&);
template void NOP::Execute<false>(CPU&, Bus&);
template void NOP::ExecuteImmediate<true>(CPU&, Bus&);
template void NOP::ExecuteImmediate<false>(CPU&, Bus&);
template void NOP::ExecuteZP<true>(CPU&, Bus&);
template void NOP::ExecuteZP<false>(CPU&, Bus&);
template void NOP::ExecuteZPX<true>(CPU&, Bus&);
template void NOP::ExecuteZPX<false>(CPU&, Bus&);
template void NOP::ExecuteABS<true>(CPU&, Bus&);
template void NOP::ExecuteABS<false>(CPU&, Bus&);
template void NOP::ExecuteABSX<true>(CPU&, Bus&);
template void NOP::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
