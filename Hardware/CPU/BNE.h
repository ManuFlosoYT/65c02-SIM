#ifndef SIM_65C02_BNE_H
#define SIM_65C02_BNE_H

#include "../CPU.h"
#include "../Mem.h"
class BNE {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BNE_H