#ifndef SIM_65C02_INY_H
#define SIM_65C02_INY_H

#include "../CPU.h"
#include "../Mem.h"
class INY {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_INY_H