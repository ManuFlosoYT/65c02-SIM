#include "BNE.h"

namespace Hardware::Instructions {

void BNE::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.Z == 0) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
