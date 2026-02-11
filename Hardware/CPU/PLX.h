#ifndef SIM_65C02_PLX_H
#define SIM_65C02_PLX_H

#include "../CPU.h"
#include "../Mem.h"
class PLX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PLX_H