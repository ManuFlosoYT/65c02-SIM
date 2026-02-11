#ifndef SIM_65C02_LDY_H
#define SIM_65C02_LDY_H

#include "../CPU.h"
#include "../Mem.h"

class LDY {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarZPX(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
    static void EjecutarABSX(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_LDY_H