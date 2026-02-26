#include "BRA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void BRA::Execute(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    cpu.PC += offset;
    cpu.UpdatePagePtr(bus);
}

template void BRA::Execute<true>(CPU&, Bus&);
template void BRA::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
