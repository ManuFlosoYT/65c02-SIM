#ifndef SIM_65C02_TXS_H
#define SIM_65C02_TXS_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class TXS {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TXS_H