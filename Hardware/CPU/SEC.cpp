#include "SEC.h"

namespace Hardware::Instructions {

void SEC::Execute(CPU& cpu, Mem& mem) {
    cpu.C = 1;
}

}  // namespace Hardware::Instructions
