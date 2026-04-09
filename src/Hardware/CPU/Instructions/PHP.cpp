#include "PHP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void PHP::Execute(CPU& cpu, Bus& bus) {    
    cpu.PushByte<Debug>(cpu.GetStatus(), bus);
}

template void PHP::Execute<true>(CPU&, Bus&);
template void PHP::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
