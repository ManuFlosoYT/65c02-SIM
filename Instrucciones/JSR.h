#ifndef MIPS_SIM_65C02_JSR_H
#define MIPS_SIM_65C02_JSR_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class JSR {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_JSR_H