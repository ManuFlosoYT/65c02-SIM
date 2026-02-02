#ifndef SIM_65C02_SEC_H
#define SIM_65C02_SEC_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class SEC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SEC_H