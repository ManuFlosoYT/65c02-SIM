#ifndef SIM_65C02_RTS_H
#define SIM_65C02_RTS_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class RTS {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_RTS_H