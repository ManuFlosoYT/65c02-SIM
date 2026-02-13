#ifndef SIM_65C02_CLV_H
#define SIM_65C02_CLV_H

#include "../CPU.h"
#include "../Mem.h"
class CLV {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLV_H