#include "JMP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void JMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord(bus);
    cpu.PC = dir;
}

void JMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord(bus);
    dir += cpu.X;
    cpu.PC = dir;
}

void JMP::ExecuteIND(CPU& cpu, Bus& bus) {
    Word dirIND = cpu.FetchWord(bus);
    Word dir = cpu.ReadWord(dirIND, bus);
    cpu.PC = dir;
}

}  // namespace Hardware::Instructions
