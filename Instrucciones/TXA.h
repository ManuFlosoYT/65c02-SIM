#ifndef MIPS_SIM_65C02_TXA_H
#define MIPS_SIM_65C02_TXA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TXA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_TXA_H