#include "PHY.h"

namespace Hardware::Instructions {

void PHY::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.Y, bus);
}

}  // namespace Hardware::Instructions
