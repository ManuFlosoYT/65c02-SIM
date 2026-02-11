#ifndef SIM_65C02_PHA_H
#define SIM_65C02_PHA_H

#include "../CPU.h"
#include "../Mem.h"
class PHA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHA_H