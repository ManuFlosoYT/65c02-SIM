#include "CLV.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void CLV::Execute(CPU& cpu, Bus& bus) {
    cpu.V = 0;
}

template void CLV::Execute<true>(CPU&, Bus&);
template void CLV::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
