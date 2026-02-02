#ifndef SIM_65C02_CLD_H
#define SIM_65C02_CLD_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class CLD {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLD_H