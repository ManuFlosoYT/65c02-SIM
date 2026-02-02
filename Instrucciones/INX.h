#ifndef SIM_65C02_INX_H
#define SIM_65C02_INX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class INX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_INX_H