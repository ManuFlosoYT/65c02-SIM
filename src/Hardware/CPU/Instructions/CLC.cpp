#include "CLC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void CLC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 0;
}

template void CLC::Execute<true>(CPU&, Bus&);
template void CLC::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
