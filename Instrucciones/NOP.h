#ifndef MIPS_SIM_65C02_NOP_H
#define MIPS_SIM_65C02_NOP_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class NOP{
    public:
        void Ejecutar( CPU& cpu, Mem& mem ) const;
};

#endif //MIPS_SIM_65C02_NOP_H