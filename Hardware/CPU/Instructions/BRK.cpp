#include "BRK.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BRK::Execute(CPU& cpu, Bus& bus) {
    cpu.B = true;
    cpu.PushWord<Debug>(cpu.PC + 1, bus);
    Byte processorStatus = cpu.GetStatus();
    cpu.PushByte<Debug>(processorStatus, bus);
    cpu.PC = cpu.ReadWord<Debug>(0xFFFE, bus);
    cpu.UpdatePagePtr(bus);
}

template void BRK::Execute<true>(CPU&, Bus&);
template void BRK::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
