#ifndef SIM_65C02_PLA_H
#define SIM_65C02_PLA_H

#include "../CPU.h"
#include "../Mem.h"
class PLA {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLA_H