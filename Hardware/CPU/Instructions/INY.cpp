#include "INY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void INY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y++;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template void INY::Execute<true>(CPU&, Bus&);
template void INY::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
