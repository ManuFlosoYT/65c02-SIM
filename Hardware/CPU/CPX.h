#ifndef SIM_65C02_CPX_H
#define SIM_65C02_CPX_H

#include "../CPU.h"
#include "../Mem.h"

class CPX {
public:
    static void ExecuteImmediate(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CPX_H