#ifndef SIM_65C02_BCS_H
#define SIM_65C02_BCS_H

#include "../CPU.h"
#include "../Mem.h"
class BCS {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BCS_H