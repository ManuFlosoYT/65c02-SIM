#ifndef SIM_65C02_LDA_H
#define SIM_65C02_LDA_H

#include "../CPU.h"
#include "../Mem.h"

class LDA {
public:
    static void ExecuteImmediate(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
    static void ExecuteABSY(CPU& cpu, Mem& mem);
    static void ExecuteINDX(CPU& cpu, Mem& mem);
    static void ExecuteINDY(CPU& cpu, Mem& mem);
    static void ExecuteIND_ZP(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_LDA_H