#include "TRB.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug>
void TRB::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write<Debug>(ZP_Dir, res2);
}

template <bool Debug>
void TRB::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write<Debug>(Dir, res2);
}

template void TRB::ExecuteZP<true>(CPU&, Bus&);
template void TRB::ExecuteZP<false>(CPU&, Bus&);
template void TRB::ExecuteABS<true>(CPU&, Bus&);
template void TRB::ExecuteABS<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
