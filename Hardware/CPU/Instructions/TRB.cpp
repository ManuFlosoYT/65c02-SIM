#include "TRB.h"

namespace Hardware::Instructions {

void TRB::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write(ZP_Dir, res2);
}

void TRB::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    bus.Write(Dir, res2);
}

}  // namespace Hardware::Instructions
