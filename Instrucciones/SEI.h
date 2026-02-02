#ifndef SIM_65C02_SEI_H
#define SIM_65C02_SEI_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class SEI {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SEI_H