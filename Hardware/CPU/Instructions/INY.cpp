#include "INY.h"

namespace Hardware::Instructions {

void INY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y++;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
