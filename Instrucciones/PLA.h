#ifndef SIM_65C02_PLA_H
#define SIM_65C02_PLA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PLA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLA_H