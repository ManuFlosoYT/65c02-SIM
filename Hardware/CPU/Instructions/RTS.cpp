#include "RTS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void RTS::Execute(CPU& cpu, Bus& bus) {
    Word dir = cpu.PopWord<Debug>(bus) + 1;
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template void RTS::Execute<true>(CPU&, Bus&);
template void RTS::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
