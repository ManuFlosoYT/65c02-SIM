#include "PLY.h"

namespace Hardware::Instructions {

void PLY::Execute(CPU& cpu, Mem& mem) {
    cpu.Y = cpu.PopByte(mem);
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
