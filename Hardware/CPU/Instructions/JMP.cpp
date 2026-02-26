#include "JMP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void JMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord<Debug>(bus);
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
void JMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord<Debug>(bus);
    dir += cpu.X;
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
void JMP::ExecuteIND(CPU& cpu, Bus& bus) {
    Word dirIND = cpu.FetchWord<Debug>(bus);
    Word dir = cpu.ReadWord<Debug>(dirIND, bus);
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template void JMP::ExecuteABS<true>(CPU&, Bus&);
template void JMP::ExecuteABS<false>(CPU&, Bus&);
template void JMP::ExecuteABSX<true>(CPU&, Bus&);
template void JMP::ExecuteABSX<false>(CPU&, Bus&);
template void JMP::ExecuteIND<true>(CPU&, Bus&);
template void JMP::ExecuteIND<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
