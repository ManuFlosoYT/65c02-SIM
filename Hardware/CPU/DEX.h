#ifndef SIM_65C02_DEX_H
#define SIM_65C02_DEX_H

#include "../CPU.h"
#include "../Mem.h"
class DEX {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_DEX_H