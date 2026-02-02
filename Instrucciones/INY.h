#ifndef SIM_65C02_INY_H
#define SIM_65C02_INY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class INY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_INY_H