#ifndef SIM_65C02_BRA_H
#define SIM_65C02_BRA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class BRA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BRA_H
