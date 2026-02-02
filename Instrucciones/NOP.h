#ifndef SIM_65C02_NOP_H
#define SIM_65C02_NOP_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class NOP {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_NOP_H