#include "INX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void INX::Execute(CPU& cpu, Bus& bus) {
    cpu.X++;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template void INX::Execute<true>(CPU&, Bus&);
template void INX::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
