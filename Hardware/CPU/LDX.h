#ifndef SIM_65C02_LDX_H
#define SIM_65C02_LDX_H

#include "../CPU.h"
#include "../Mem.h"

class LDX {
public:
    static void ExecuteImmediate(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPY(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSY(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_LDX_H