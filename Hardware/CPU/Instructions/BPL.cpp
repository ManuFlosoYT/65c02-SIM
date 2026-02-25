#include "BPL.h"

namespace Hardware::Instructions {

void BPL::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.N == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
