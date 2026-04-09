#include "TXS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void TXS::Execute(CPU& cpu, Bus& bus) { 
    cpu.SP = 0x0100 | cpu.X; 
}

template void TXS::Execute<true>(CPU&, Bus&);
template void TXS::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
