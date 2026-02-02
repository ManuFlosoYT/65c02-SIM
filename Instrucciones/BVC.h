#ifndef SIM_65C02_BVC_H
#define SIM_65C02_BVC_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BVC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BVC_H