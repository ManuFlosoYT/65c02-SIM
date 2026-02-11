#ifndef SIM_65C02_BRA_H
#define SIM_65C02_BRA_H

#include "../CPU.h"
#include "../Mem.h"

class BRA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BRA_H
