#include "PHY.h"

namespace Hardware::Instructions {

void PHY::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.Y, mem);
}

}  // namespace Hardware::Instructions
