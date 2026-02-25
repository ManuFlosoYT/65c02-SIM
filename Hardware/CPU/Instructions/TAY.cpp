#include "TAY.h"

namespace Hardware::Instructions {

void TAY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y = cpu.A;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
