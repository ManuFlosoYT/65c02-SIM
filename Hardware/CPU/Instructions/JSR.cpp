#include "JSR.h"

namespace Hardware::Instructions {

void JSR::Execute(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Word RetDir = cpu.PC - 1;

    cpu.PushWord(RetDir, bus);

    cpu.PC = Dir;
}

}  // namespace Hardware::Instructions
