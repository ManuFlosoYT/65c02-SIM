#include "DEX.h"

namespace Hardware::Instructions {

void DEX::Execute(CPU& cpu, Bus& bus) {
    cpu.X--;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
