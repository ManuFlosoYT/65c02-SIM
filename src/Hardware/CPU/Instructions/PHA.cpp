#include "PHA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
void PHA::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.A, bus);
}

template void PHA::Execute<true>(CPU&, Bus&);
template void PHA::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
