#ifndef SIM_65C02_CLI_H
#define SIM_65C02_CLI_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class CLI {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_CLI_H