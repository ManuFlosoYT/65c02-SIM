#ifndef SIM_65C02_PHX_H
#define SIM_65C02_PHX_H

#include "../CPU.h"
#include "../Mem.h"
class PHX {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHX_H