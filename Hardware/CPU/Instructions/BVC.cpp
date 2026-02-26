#include "BVC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BVC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.V == 0) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BVC::Execute<true>(CPU&, Bus&);
template void BVC::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
