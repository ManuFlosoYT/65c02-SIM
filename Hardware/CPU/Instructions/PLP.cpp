#include "PLP.h"

namespace Hardware::Instructions {

void PLP::Execute(CPU& cpu, Mem& mem) {
    Byte status = cpu.PopByte(mem);
    cpu.SetStatus(status);
}

}  // namespace Hardware::Instructions
