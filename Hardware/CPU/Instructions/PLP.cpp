#include "PLP.h"

namespace Hardware::Instructions {

void PLP::Execute(CPU& cpu, Bus& bus) {
    Byte status = cpu.PopByte(bus);
    cpu.SetStatus(status);
}

}  // namespace Hardware::Instructions
