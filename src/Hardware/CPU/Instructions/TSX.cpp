#include "TSX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void TSX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.SP;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template void TSX::Execute<true>(CPU&, Bus&);
template void TSX::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
