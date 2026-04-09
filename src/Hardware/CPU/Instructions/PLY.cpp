#include "PLY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PLY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template void PLY::Execute<true>(CPU&, Bus&);
template void PLY::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
