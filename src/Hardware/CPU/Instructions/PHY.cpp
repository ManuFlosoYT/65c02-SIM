#include "PHY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PHY::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.Y, bus);
}

template void PHY::Execute<true>(CPU&, Bus&);
template void PHY::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
