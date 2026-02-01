#ifndef MIPS_SIM_65C02_PLX_H
#define MIPS_SIM_65C02_PLX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PLX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_PLX_H