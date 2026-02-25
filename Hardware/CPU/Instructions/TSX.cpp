#include "TSX.h"

namespace Hardware::Instructions {

void TSX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.SP;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
