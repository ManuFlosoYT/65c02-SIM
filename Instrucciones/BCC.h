#ifndef SIM_65C02_BCC_H
#define SIM_65C02_BCC_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BCC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BCC_H