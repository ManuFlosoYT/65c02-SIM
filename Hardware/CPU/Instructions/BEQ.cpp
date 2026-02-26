#include "BEQ.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BEQ::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.Z == 1) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BEQ::Execute<true>(CPU&, Bus&);
template void BEQ::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
