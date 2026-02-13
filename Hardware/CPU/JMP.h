#ifndef SIM_65C02_JMP_H
#define SIM_65C02_JMP_H

#include "../CPU.h"
#include "../Mem.h"

class JMP {
public:
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteIND(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_JMP_H