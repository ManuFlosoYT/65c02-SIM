#ifndef MIPS_SIM_65C02_STX_H
#define MIPS_SIM_65C02_STX_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class STX {
    public:
        static void EjecutarZP( CPU& cpu, Mem& mem );
        static void EjecutarZPY( CPU& cpu, Mem& mem );
        static void EjecutarABS( CPU& cpu, Mem& mem );
};

#endif //MIPS_SIM_65C02_STX_H