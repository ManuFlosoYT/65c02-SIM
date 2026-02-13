#ifndef SIM_65C02_TXA_H
#define SIM_65C02_TXA_H

#include "../CPU.h"
#include "../Mem.h"
class TXA {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TXA_H