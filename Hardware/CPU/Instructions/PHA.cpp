#include "PHA.h"

namespace Hardware::Instructions {

void PHA::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.A, bus);
}

}  // namespace Hardware::Instructions
