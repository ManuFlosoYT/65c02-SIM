#ifndef SIM_65C02_NOP_H
#define SIM_65C02_NOP_H

#include "../CPU.h"
#include "../Mem.h"
class NOP {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_NOP_H