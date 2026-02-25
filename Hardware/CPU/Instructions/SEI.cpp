#include "SEI.h"

namespace Hardware::Instructions {

void SEI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 1;
}

}  // namespace Hardware::Instructions
