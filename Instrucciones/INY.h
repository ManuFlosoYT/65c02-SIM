#ifndef MIPS_SIM_65C02_INY_H
#define MIPS_SIM_65C02_INY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class INY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_INY_H