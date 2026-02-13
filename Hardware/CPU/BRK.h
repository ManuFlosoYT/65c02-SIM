#ifndef SIM_65C02_BRK_H
#define SIM_65C02_BRK_H

#include "../CPU.h"
#include "../Mem.h"
class BRK {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BRK_H