#include "BNE.h"

namespace Hardware::Instructions {

void BNE::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.Z == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
