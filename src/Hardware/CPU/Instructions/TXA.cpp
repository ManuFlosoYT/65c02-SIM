#include "TXA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void TXA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.X;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template void TXA::Execute<true>(CPU&, Bus&);
template void TXA::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
