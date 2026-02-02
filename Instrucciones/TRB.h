#ifndef SIM_65C02_TRB_H
#define SIM_65C02_TRB_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class TRB {
public:
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TRB_H