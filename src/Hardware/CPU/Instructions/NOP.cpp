#include "NOP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void NOP::Execute(CPU& cpu, Bus& bus) {
    // Bruh, you really thought NOP would do something lmao 😂😂😂😂
}

template void NOP::Execute<true>(CPU&, Bus&);
template void NOP::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
