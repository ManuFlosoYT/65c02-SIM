#ifndef SIM_65C02_CPY_H
#define SIM_65C02_CPY_H

#include "../CPU.h"
#include "../Mem.h"

class CPY {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CPY_H