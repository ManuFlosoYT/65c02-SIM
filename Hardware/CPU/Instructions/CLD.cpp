#include "CLD.h"

namespace Hardware::Instructions {

void CLD::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 0;
}

}  // namespace Hardware::Instructions
