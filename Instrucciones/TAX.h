#ifndef MIPS_SIM_65C02_TAX_H
#define MIPS_SIM_65C02_TAX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TAX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_TAX_H