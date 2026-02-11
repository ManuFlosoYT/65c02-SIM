#ifndef SIM_65C02_TRB_H
#define SIM_65C02_TRB_H

#include "../CPU.h"
#include "../Mem.h"

class TRB {
public:
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TRB_H