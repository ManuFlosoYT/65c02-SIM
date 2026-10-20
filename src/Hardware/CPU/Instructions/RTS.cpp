#include "RTS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Core/SubroutineProfiler.h"

namespace Hardware::Instructions {

template <bool Debug>
void RTS::Execute(CPU& cpu, Bus& bus) {
    if constexpr (Debug) {
        if (cpu.GetProfiler() != nullptr) {
            cpu.GetProfiler()->OnRTS(cpu.GetCycleCount());
        }
    }
    Word dir = cpu.PopWord<Debug>(bus) + 1;
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template void RTS::Execute<true>(CPU&, Bus&);
template void RTS::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
