#include "BCC.h"

namespace Hardware::Instructions {

void BCC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.C == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
