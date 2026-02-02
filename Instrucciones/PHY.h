#ifndef SIM_65C02_PHY_H
#define SIM_65C02_PHY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PHY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHY_H