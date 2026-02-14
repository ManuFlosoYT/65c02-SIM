#include "BVC.h"

namespace Hardware::Instructions {

void BVC::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.V == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
