#ifndef SIM_65C02_BEQ_H
#define SIM_65C02_BEQ_H

#include "../CPU.h"
#include "../Mem.h"
class BEQ {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BEQ_H