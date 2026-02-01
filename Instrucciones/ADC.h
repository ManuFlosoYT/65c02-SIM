#ifndef MIPS_SIM_65C02_ADC_H
#define MIPS_SIM_65C02_ADC_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class ADC {
    public:
        static void EjecutarInmediato( CPU& cpu, Mem& mem );
        static void EjecutarZP( CPU& cpu, Mem& mem );
        static void EjecutarZPX( CPU& cpu, Mem& mem );
        static void EjecutarABS( CPU& cpu, Mem& mem );
        static void EjecutarABSX( CPU& cpu, Mem& mem );
        static void EjecutarABSY( CPU& cpu, Mem& mem );
        static void EjecutarINDX( CPU& cpu, Mem& mem );
        static void EjecutarINDY( CPU& cpu, Mem& mem );
};

#endif //MIPS_SIM_65C02_ADC_H