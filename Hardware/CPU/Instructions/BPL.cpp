#include "BPL.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BPL::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.N == 0) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BPL::Execute<true>(CPU&, Bus&);
template void BPL::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
