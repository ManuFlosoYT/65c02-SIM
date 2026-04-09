#include "PHX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PHX::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.X, bus);
}

template void PHX::Execute<true>(CPU&, Bus&);
template void PHX::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
