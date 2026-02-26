#include "SEC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void SEC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 1;
}

template void SEC::Execute<true>(CPU&, Bus&);
template void SEC::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
