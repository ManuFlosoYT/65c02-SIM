#ifndef SIM_65C02_TSX_H
#define SIM_65C02_TSX_H

#include "../CPU.h"
#include "../Mem.h"
class TSX {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_TSX_H