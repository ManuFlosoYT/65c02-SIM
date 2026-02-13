#ifndef SIM_65C02_STY_H
#define SIM_65C02_STY_H

#include "../CPU.h"
#include "../Mem.h"

class STY {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_STY_H