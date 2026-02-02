#ifndef SIM_65C02_CPX_H
#define SIM_65C02_CPX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class CPX {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CPX_H