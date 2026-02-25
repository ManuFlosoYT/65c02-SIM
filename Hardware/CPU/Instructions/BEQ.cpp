#include "BEQ.h"

namespace Hardware::Instructions {

void BEQ::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.Z == 1) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
