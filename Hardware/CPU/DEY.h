#ifndef SIM_65C02_DEY_H
#define SIM_65C02_DEY_H

#include "../CPU.h"
#include "../Mem.h"
class DEY {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_DEY_H