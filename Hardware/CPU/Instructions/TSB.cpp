#include "TSB.h"

namespace Hardware::Instructions {

void TSB::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = cpu.A | dato;
    mem.Write(ZP_Dir, res2);
}

void TSB::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = cpu.A | dato;
    mem.Write(Dir, res2);
}

}  // namespace Hardware::Instructions
