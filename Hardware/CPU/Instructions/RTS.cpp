#include "RTS.h"

namespace Hardware::Instructions {

void RTS::Execute(CPU& cpu, Mem& mem) {
    Word dir = cpu.PopWord(mem) + 1;
    cpu.PC = dir;
}

}  // namespace Hardware::Instructions
