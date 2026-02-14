#include "STZ.h"

namespace Hardware::Instructions {

void STZ::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem.Write(ZP_Dir, 0);
}

void STZ::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    mem.Write(ZP_Dir, 0);
}

void STZ::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem.Write(Dir, 0);
}

void STZ::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    mem.Write(Dir, 0);
}

}  // namespace Hardware::Instructions
