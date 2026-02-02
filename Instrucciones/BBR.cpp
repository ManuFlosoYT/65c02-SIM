#include "BBR.h"

void BBR::Ejecutar0(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000001) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar1(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000010) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar2(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000100) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar3(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00001000) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar4(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00010000) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar5(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00100000) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar6(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b01000000) == 0) {
        cpu.PC = dir;
    }
}

void BBR::Ejecutar7(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b10000000) == 0) {
        cpu.PC = dir;
    }
}
