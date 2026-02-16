#include "CLD.h"

namespace Hardware::Instructions {

void CLD::Execute(CPU& cpu, Mem& mem) {
    cpu.D = 0;
}

}  // namespace Hardware::Instructions
