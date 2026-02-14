#include "STA.h"

namespace Hardware::Instructions {

void STA::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    mem.Write(ZP_Dir, cpu.A);
}

void STA::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    mem.Write(ZP_Dir, cpu.A);
}

void STA::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    mem.Write(Dir, cpu.A);
}

void STA::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    mem.Write(Dir, cpu.A);
}

void STA::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    mem.Write(Dir, cpu.A);
}

void STA::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWord(ZP_Dir, mem);
    mem.Write(Dir, cpu.A);
}

void STA::ExecuteINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    mem.Write(dir, cpu.A);
}


void STA::ExecuteINDZP( CPU& cpu, Mem& mem){
Byte ZP_Dir = cpu.FetchByte(mem);
    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    mem.Write(dir, cpu.A);
}

}  // namespace Hardware::Instructions
