#ifndef SIM_65C02_ORA_H
#define SIM_65C02_ORA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class ORA {
public:
    static void EjecutarInmediato(CPU& cpu, Mem& mem);
    static void EjecutarZP(CPU& cpu, Mem& mem);
    static void EjecutarZPX(CPU& cpu, Mem& mem);
    static void EjecutarABS(CPU& cpu, Mem& mem);
    static void EjecutarABSX(CPU& cpu, Mem& mem);
    static void EjecutarABSY(CPU& cpu, Mem& mem);
    static void EjecutarINDX(CPU& cpu, Mem& mem);
    static void EjecutarINDY(CPU& cpu, Mem& mem);
    static void EjecutarIND_ZP(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_ORA_H