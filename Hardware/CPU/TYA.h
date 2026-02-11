#ifndef SIM_65C02_TYA_H
#define SIM_65C02_TYA_H

#include "../CPU.h"
#include "../Mem.h"
class TYA {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TYA_H