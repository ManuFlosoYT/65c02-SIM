#ifndef MIPS_SIM_65C02_RTS_H
#define MIPS_SIM_65C02_RTS_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class RTS {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // MIPS_SIM_65C02_RTS_H