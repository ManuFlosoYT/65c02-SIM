#ifndef SIM_65C02_BPL_H
#define SIM_65C02_BPL_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BPL {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BPL_H