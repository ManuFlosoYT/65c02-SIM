#ifndef SIM_65C02_PHA_H
#define SIM_65C02_PHA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PHA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHA_H