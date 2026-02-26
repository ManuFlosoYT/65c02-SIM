#include "BNE.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BNE::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.Z == 0) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BNE::Execute<true>(CPU&, Bus&);
template void BNE::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
