#ifndef SIM_65C02_CLV_H
#define SIM_65C02_CLV_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class CLV {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLV_H