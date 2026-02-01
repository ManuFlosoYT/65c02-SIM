#ifndef MIPS_SIM_65C02_LSR_H
#define MIPS_SIM_65C02_LSR_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class LSR {
    public:
        static void EjecutarAcumulador( CPU& cpu, Mem& mem );
        static void EjecutarZP( CPU& cpu, Mem& mem );
        static void EjecutarZPX( CPU& cpu, Mem& mem );
        static void EjecutarABS( CPU& cpu, Mem& mem );
        static void EjecutarABSX( CPU& cpu, Mem& mem );
};

#endif //MIPS_SIM_65C02_LSR_H