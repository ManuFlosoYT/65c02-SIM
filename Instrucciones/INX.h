#ifndef MIPS_SIM_65C02_INX_H
#define MIPS_SIM_65C02_INX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class INX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_INX_H