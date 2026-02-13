#ifndef SIM_65C02_PLP_H
#define SIM_65C02_PLP_H

#include "../CPU.h"
#include "../Mem.h"
class PLP {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLP_H