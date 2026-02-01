#ifndef MIPS_SIM_65C02_DEY_H
#define MIPS_SIM_65C02_DEY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class DEY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_DEY_H