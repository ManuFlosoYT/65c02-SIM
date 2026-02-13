#ifndef SIM_65C02_CLI_H
#define SIM_65C02_CLI_H

#include "../CPU.h"
#include "../Mem.h"
class CLI {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLI_H