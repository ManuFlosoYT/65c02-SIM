#ifndef MIPS_SIM_65C02_TAY_H
#define MIPS_SIM_65C02_TAY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TAY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_TAY_H