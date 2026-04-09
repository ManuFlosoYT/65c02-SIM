#include "STZ.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void STZ::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, 0);
}

template <bool Debug>
void STZ::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    bus.Write<Debug>(ZP_Dir, 0);
}

template <bool Debug>
void STZ::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    bus.Write<Debug>(Dir, 0);
}

template <bool Debug>
void STZ::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Dir += cpu.X;
    bus.Write<Debug>(Dir, 0);
}

template void STZ::ExecuteZP<true>(CPU&, Bus&);
template void STZ::ExecuteZP<false>(CPU&, Bus&);
template void STZ::ExecuteZPX<true>(CPU&, Bus&);
template void STZ::ExecuteZPX<false>(CPU&, Bus&);
template void STZ::ExecuteABS<true>(CPU&, Bus&);
template void STZ::ExecuteABS<false>(CPU&, Bus&);
template void STZ::ExecuteABSX<true>(CPU&, Bus&);
template void STZ::ExecuteABSX<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
