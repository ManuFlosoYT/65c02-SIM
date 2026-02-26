#include "BCS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BCS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.C == 1) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BCS::Execute<true>(CPU&, Bus&);
template void BCS::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
