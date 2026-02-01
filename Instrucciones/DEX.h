#ifndef MIPS_SIM_65C02_DEX_H
#define MIPS_SIM_65C02_DEX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class DEX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_DEX_H