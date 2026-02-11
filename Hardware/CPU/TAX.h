#ifndef SIM_65C02_TAX_H
#define SIM_65C02_TAX_H

#include "../CPU.h"
#include "../Mem.h"
class TAX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TAX_H