#ifndef SIM_65C02_JSR_H
#define SIM_65C02_JSR_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class JSR {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_JSR_H