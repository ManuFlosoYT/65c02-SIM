#ifndef SIM_65C02_INX_H
#define SIM_65C02_INX_H

#include "../CPU.h"
#include "../Mem.h"
class INX {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_INX_H