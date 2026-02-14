#include "BVS.h"

namespace Hardware::Instructions {

void BVS::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.V == 1) {
        cpu.PC += dir;
    }
}

}  // namespace Hardware::Instructions
