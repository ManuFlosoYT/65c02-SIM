#include "BVC.h"

namespace Hardware::Instructions {

void BVC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.V == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
