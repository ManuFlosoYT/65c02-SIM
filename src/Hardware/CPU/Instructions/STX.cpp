#include "STX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void STX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.X);
}

template <bool Debug>
void STX::ExecuteZPY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.Y;
    bus.Write<Debug>(ZP_Dir, cpu.X);
}

template <bool Debug>
void STX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.X);
}

template void STX::ExecuteZP<true>(CPU&, Bus&);
template void STX::ExecuteZP<false>(CPU&, Bus&);
template void STX::ExecuteZPY<true>(CPU&, Bus&);
template void STX::ExecuteZPY<false>(CPU&, Bus&);
template void STX::ExecuteABS<true>(CPU&, Bus&);
template void STX::ExecuteABS<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
