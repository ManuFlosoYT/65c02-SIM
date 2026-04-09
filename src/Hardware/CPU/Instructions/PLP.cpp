#include "PLP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PLP::Execute(CPU& cpu, Bus& bus) {
    Byte status = cpu.PopByte<Debug>(bus);
    cpu.SetStatus(status);
}

template void PLP::Execute<true>(CPU&, Bus&);
template void PLP::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
