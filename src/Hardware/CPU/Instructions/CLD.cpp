#include "CLD.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void CLD::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 0;
}

template void CLD::Execute<true>(CPU&, Bus&);
template void CLD::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
