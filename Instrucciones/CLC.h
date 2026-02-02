#ifndef SIM_65C02_CLC_H
#define SIM_65C02_CLC_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class CLC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLC_H