#ifndef SIM_65C02_BIT_H
#define SIM_65C02_BIT_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class BIT {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarZPX(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
    static void EjecutarABSX(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BIT_H