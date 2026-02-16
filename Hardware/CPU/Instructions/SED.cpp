#include "SED.h"

namespace Hardware::Instructions {

void SED::Execute(CPU& cpu, Mem& mem) {
    cpu.D = 1;
}

}  // namespace Hardware::Instructions
