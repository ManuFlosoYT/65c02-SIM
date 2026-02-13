#ifndef SIM_65C02_ASL_H
#define SIM_65C02_ASL_H

#include "../CPU.h"
#include "../Mem.h"

class ASL {
public:
    static void ExecuteAccumulator(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_ASL_H