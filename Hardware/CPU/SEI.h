#ifndef SIM_65C02_SEI_H
#define SIM_65C02_SEI_H

#include "../CPU.h"
#include "../Mem.h"
class SEI {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SEI_H