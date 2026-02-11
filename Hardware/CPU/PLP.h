#ifndef SIM_65C02_PLP_H
#define SIM_65C02_PLP_H

#include "../CPU.h"
#include "../Mem.h"
class PLP {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLP_H