#include "PLX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PLX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template void PLX::Execute<true>(CPU&, Bus&);
template void PLX::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
