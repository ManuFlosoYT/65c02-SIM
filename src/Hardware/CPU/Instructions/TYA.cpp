#include "TYA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void TYA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.Y;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template void TYA::Execute<true>(CPU&, Bus&);
template void TYA::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
