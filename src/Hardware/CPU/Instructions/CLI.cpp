#include "CLI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void CLI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 0;
}

template void CLI::Execute<true>(CPU&, Bus&);
template void CLI::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
