#ifndef SIM_65C02_PHP_H
#define SIM_65C02_PHP_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class PHP {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_PHP_H