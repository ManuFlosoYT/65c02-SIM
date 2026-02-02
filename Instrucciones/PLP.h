#ifndef SIM_65C02_PLP_H
#define SIM_65C02_PLP_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PLP {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLP_H