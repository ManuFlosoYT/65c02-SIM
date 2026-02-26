#include "SEI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void SEI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 1;
}

template void SEI::Execute<true>(CPU&, Bus&);
template void SEI::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
