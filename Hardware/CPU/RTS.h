#ifndef SIM_65C02_RTS_H
#define SIM_65C02_RTS_H

#include "../CPU.h"
#include "../Mem.h"
class RTS {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_RTS_H