#include "LDA.h"

void LDA::EjecutarInmediato(CPU &cpu, Mem &mem) const {
    Byte dato = cpu.FetchByte(mem);
    cpu.A = dato;

    cpu.Z = (dato == 0);
    cpu.N = (dato & 0b10000000) > 0;
}

void LDA::EjecutarZP(CPU &cpu, Mem &mem) const {
    // LDA ZP
}

void LDA::EjecutarZPX(CPU &cpu, Mem &mem) const {
    // LDA ZPX
}

void LDA::EjecutarABS(CPU &cpu, Mem &mem) const {
    // LDA ABS
}

void LDA::EjecutarABSX(CPU &cpu, Mem &mem) const {
    // LDA ABSX
}

void LDA::EjecutarABSY(CPU &cpu, Mem &mem) const {
    // LDA ABSY
}

void LDA::EjecutarINDX(CPU &cpu, Mem &mem) const {
    // LDA INDX
}

void LDA::EjecutarINDY(CPU &cpu, Mem &mem) const {
    // LDA INDY
}
