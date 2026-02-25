#include "PHX.h"

namespace Hardware::Instructions {

void PHX::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.X, bus);
}

}  // namespace Hardware::Instructions
