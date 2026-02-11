#ifndef SIM_65C02_SED_H
#define SIM_65C02_SED_H

#include "../CPU.h"
#include "../Mem.h"
class SED {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SED_H