#include "CLV.h"

namespace Hardware::Instructions {

void CLV::Execute(CPU& cpu, Mem& mem) {
    cpu.V = 0;
}

}  // namespace Hardware::Instructions
