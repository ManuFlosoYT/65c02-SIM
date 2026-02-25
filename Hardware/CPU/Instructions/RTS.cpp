#include "RTS.h"

namespace Hardware::Instructions {

void RTS::Execute(CPU& cpu, Bus& bus) {
    Word dir = cpu.PopWord(bus) + 1;
    cpu.PC = dir;
}

}  // namespace Hardware::Instructions
