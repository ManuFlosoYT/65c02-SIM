#include "SEC.h"

namespace Hardware::Instructions {

void SEC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 1;
}

}  // namespace Hardware::Instructions
