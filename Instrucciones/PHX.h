#ifndef MIPS_SIM_65C02_PHX_H
#define MIPS_SIM_65C02_PHX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PHX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_PHX_H