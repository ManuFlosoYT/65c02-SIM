#include "SED.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void SED::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 1;
}

template void SED::Execute<true>(CPU&, Bus&);
template void SED::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
