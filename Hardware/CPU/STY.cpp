#include "STY.h"

namespace Hardware::Instructions {

void STY::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem.Write(ZP_Dir, cpu.Y);
}

void STY::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    mem.Write(ZP_Dir, cpu.Y);
}

void STY::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem.Write(Dir, cpu.Y);
}

}  // namespace Hardware::Instructions
