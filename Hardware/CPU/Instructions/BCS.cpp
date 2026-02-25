#include "BCS.h"

namespace Hardware::Instructions {

void BCS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.C == 1) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
