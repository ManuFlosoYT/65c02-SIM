#ifndef SIM_65C02_PHY_H
#define SIM_65C02_PHY_H

#include "../CPU.h"
#include "../Mem.h"
class PHY {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHY_H