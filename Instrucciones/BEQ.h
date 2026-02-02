#ifndef SIM_65C02_BEQ_H
#define SIM_65C02_BEQ_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BEQ {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BEQ_H