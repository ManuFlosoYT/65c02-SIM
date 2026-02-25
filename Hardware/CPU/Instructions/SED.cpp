#include "SED.h"

namespace Hardware::Instructions {

void SED::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 1;
}

}  // namespace Hardware::Instructions
