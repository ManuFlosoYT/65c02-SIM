#include "SMB.h"

void SMB::Ejecutar0(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00000001;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar1(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00000010;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar2(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00000100;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar3(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00001000;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar4(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00010000;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar5(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b00100000;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar6(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b01000000;
    mem[ZP_Dir] = data;
}

void SMB::Ejecutar7(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte data = cpu.LeerByte(ZP_Dir, mem);
    data |= 0b10000000;
    mem[ZP_Dir] = data;
}
