#ifndef SIM_65C02_LDX_H
#define SIM_65C02_LDX_H

#include "../CPU.h"
#include "../Mem.h"

class LDX {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarZPY(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
    static void EjecutarABSY(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_LDX_H