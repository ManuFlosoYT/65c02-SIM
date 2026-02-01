#include "ADC.h"

static void SetFlags(CPU& cpu, Word res, Byte dato) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = (res > 0xFF);
    cpu.V = ((cpu.A ^ dato) & 0b10000000) != 0;
}

void ADC::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}

void ADC::EjecutarINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    Byte dato = cpu.LeerByte(dir, mem);
    Word res = cpu.A + dato + cpu.C;
    cpu.A = res;

    SetFlags(cpu, res, dato);
}