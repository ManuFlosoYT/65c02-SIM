#include "RTI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void RTI::Execute(CPU& cpu, Bus& bus) {
    Byte status = cpu.PopByte<Debug>(bus);
    cpu.SetStatus(status);
    cpu.PC = cpu.PopWord<Debug>(bus);
    cpu.UpdatePagePtr(bus);
}

template void RTI::Execute<true>(CPU&, Bus&);
template void RTI::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
