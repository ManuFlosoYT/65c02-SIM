#ifndef SIM_65C02_PLY_H
#define SIM_65C02_PLY_H

#include "../CPU.h"
#include "../Mem.h"
class PLY {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLY_H