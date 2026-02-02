#ifndef SIM_65C02_TAY_H
#define SIM_65C02_TAY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TAY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TAY_H