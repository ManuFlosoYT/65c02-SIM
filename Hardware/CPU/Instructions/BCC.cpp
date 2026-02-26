#include "BCC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BCC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.C == 0) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BCC::Execute<true>(CPU&, Bus&);
template void BCC::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
