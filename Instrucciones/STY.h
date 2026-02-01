#ifndef MIPS_SIM_65C02_STY_H
#define MIPS_SIM_65C02_STY_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class STY {
    public:
        static void EjecutarZP( CPU& cpu, Mem& mem );
        static void EjecutarZPX( CPU& cpu, Mem& mem );
        static void EjecutarABS( CPU& cpu, Mem& mem );
};

#endif //MIPS_SIM_65C02_STY_H