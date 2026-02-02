#ifndef SIM_65C02_STY_H
#define SIM_65C02_STY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class STY {
public:
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarZPX(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_STY_H