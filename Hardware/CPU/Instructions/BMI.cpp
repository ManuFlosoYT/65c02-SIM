#include "BMI.h"

namespace Hardware::Instructions {

void BMI::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.N == 1) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
