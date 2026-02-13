#include "RMB.h"

void RMB::Execute0(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11111110;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute1(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11111101;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute2(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11111011;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute3(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11110111;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute4(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11101111;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute5(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b11011111;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute6(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b10111111;
    mem.Write(ZP_Dir, data);
}

void RMB::Execute7(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data &= 0b01111111;
    mem.Write(ZP_Dir, data);
}
