#include "STX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void STX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    bus.Write(ZP_Dir, cpu.X);
}

void STX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.Y;
    bus.Write(ZP_Dir, cpu.X);
}

void STX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    bus.Write(Dir, cpu.X);
}

}  // namespace Hardware::Instructions
