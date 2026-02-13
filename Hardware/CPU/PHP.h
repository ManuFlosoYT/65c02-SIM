#ifndef SIM_65C02_PHP_H
#define SIM_65C02_PHP_H

#include "../CPU.h"
#include "../Mem.h"
class PHP {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHP_H