#ifndef SIM_65C02_SED_H
#define SIM_65C02_SED_H

#include "../CPU.h"
#include "../Mem.h"
class SED {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SED_H