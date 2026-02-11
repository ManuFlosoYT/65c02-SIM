#ifndef SIM_65C02_BVC_H
#define SIM_65C02_BVC_H

#include "../CPU.h"
#include "../Mem.h"
class BVC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BVC_H