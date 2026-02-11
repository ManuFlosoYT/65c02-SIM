#ifndef SIM_65C02_SEC_H
#define SIM_65C02_SEC_H

#include "../CPU.h"
#include "../Mem.h"
class SEC {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SEC_H