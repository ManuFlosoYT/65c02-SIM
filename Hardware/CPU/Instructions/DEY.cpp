#include "DEY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void DEY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y--;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template void DEY::Execute<true>(CPU&, Bus&);
template void DEY::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
