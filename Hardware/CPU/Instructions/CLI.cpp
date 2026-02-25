#include "CLI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void CLI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 0;
}

}  // namespace Hardware::Instructions
