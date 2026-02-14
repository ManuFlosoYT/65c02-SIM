#include "CLC.h"

namespace Hardware::Instructions {

void CLC::Execute(CPU& cpu, Mem& mem) {
    cpu.C = 0;
}

}  // namespace Hardware::Instructions
