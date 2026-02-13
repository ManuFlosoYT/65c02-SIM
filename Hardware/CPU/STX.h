#ifndef SIM_65C02_STX_H
#define SIM_65C02_STX_H

#include "../CPU.h"
#include "../Mem.h"

class STX {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPY(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_STX_H