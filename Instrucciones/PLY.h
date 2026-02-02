#ifndef SIM_65C02_PLY_H
#define SIM_65C02_PLY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PLY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLY_H