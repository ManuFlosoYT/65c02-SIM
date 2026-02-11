#include "RMB.h"

void RMB::Ejecutar0(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11111110;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar1(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11111101;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar2(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11111011;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar3(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11110111;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar4(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11101111;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar5(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b11011111;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar6(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b10111111;
    mem.Write(ZP_Dir, data);
}

void RMB::Ejecutar7(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data &= 0b01111111;
    mem.Write(ZP_Dir, data);
}
