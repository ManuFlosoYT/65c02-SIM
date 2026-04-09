#include "PLA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PLA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template void PLA::Execute<true>(CPU&, Bus&);
template void PLA::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
