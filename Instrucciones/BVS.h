#ifndef SIM_65C02_BVS_H
#define SIM_65C02_BVS_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BVS {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BVS_H