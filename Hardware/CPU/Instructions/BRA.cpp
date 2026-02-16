#include "BRA.h"

namespace Hardware::Instructions {

void BRA::Execute(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    cpu.PC += offset;
}

}  // namespace Hardware::Instructions
