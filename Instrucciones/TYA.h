#ifndef MIPS_SIM_65C02_TYA_H
#define MIPS_SIM_65C02_TYA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TYA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_TYA_H