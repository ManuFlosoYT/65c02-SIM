#include "SMB.h"

void SMB::Execute0(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00000001;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute1(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00000010;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute2(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00000100;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute3(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00001000;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute4(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00010000;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute5(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b00100000;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute6(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b01000000;
    mem.Write(ZP_Dir, data);
}

void SMB::Execute7(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.ReadByte(ZP_Dir, mem);
    data |= 0b10000000;
    mem.Write(ZP_Dir, data);
}
