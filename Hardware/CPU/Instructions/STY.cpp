#include "STY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void STY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, cpu.Y);
}

template <bool Debug>
void STY::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, cpu.Y);
}

template <bool Debug>
void STY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, cpu.Y);
}

template void STY::ExecuteZP<true>(CPU&, Bus&);
template void STY::ExecuteZP<false>(CPU&, Bus&);
template void STY::ExecuteZPX<true>(CPU&, Bus&);
template void STY::ExecuteZPX<false>(CPU&, Bus&);
template void STY::ExecuteABS<true>(CPU&, Bus&);
template void STY::ExecuteABS<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
