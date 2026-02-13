#ifndef SIM_65C02_TXS_H
#define SIM_65C02_TXS_H

#include "../CPU.h"
#include "../Mem.h"
class TXS {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TXS_H