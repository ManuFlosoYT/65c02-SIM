#ifndef SIM_65C02_CLC_H
#define SIM_65C02_CLC_H

#include "../CPU.h"
#include "../Mem.h"
class CLC {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLC_H