#ifndef SIM_65C02_TSB_H
#define SIM_65C02_TSB_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class TSB {
public:
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TSB_H