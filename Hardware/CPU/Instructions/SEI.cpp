#include "SEI.h"

namespace Hardware::Instructions {

void SEI::Execute(CPU& cpu, Mem& mem) {
    cpu.I = 1;
}

}  // namespace Hardware::Instructions
