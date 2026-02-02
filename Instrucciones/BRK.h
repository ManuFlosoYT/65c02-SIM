#ifndef SIM_65C02_BRK_H
#define SIM_65C02_BRK_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BRK {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BRK_H