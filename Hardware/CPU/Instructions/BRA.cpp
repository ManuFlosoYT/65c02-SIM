#include "BRA.h"

namespace Hardware::Instructions {

void BRA::Execute(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    cpu.PC += offset;
}

}  // namespace Hardware::Instructions
