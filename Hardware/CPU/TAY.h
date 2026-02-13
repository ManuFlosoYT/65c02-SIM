#ifndef SIM_65C02_TAY_H
#define SIM_65C02_TAY_H

#include "../CPU.h"
#include "../Mem.h"
class TAY {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TAY_H