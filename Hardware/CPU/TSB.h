#ifndef SIM_65C02_TSB_H
#define SIM_65C02_TSB_H

#include "../CPU.h"
#include "../Mem.h"

class TSB {
public:
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TSB_H