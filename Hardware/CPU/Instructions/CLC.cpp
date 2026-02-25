#include "CLC.h"

namespace Hardware::Instructions {

void CLC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 0;
}

}  // namespace Hardware::Instructions
