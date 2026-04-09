#include "JSR.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void JSR::Execute(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Word RetDir = cpu.PC - 1;

    cpu.PushWord<Debug>(RetDir, bus);

    cpu.PC = Dir;
    cpu.UpdatePagePtr(bus);
}

template void JSR::Execute<true>(CPU&, Bus&);
template void JSR::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
