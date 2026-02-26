#include "BVS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BVS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if(cpu.V == 1) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template void BVS::Execute<true>(CPU&, Bus&);
template void BVS::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
