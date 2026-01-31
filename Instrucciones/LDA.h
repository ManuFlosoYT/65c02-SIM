#ifndef MIPS_SIM_65C02_LDA_H
#define MIPS_SIM_65C02_LDA_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"

class LDA {
    public:
        void EjecutarInmediato( CPU& cpu, Mem& mem ) const;
        void EjecutarZP( CPU& cpu, Mem& mem ) const;
        void EjecutarZPX( CPU& cpu, Mem& mem ) const;
        void EjecutarABS( CPU& cpu, Mem& mem ) const;
        void EjecutarABSX( CPU& cpu, Mem& mem ) const;
        void EjecutarABSY( CPU& cpu, Mem& mem ) const;
        void EjecutarINDX( CPU& cpu, Mem& mem ) const;
        void EjecutarINDY( CPU& cpu, Mem& mem ) const;
};

#endif //MIPS_SIM_65C02_LDA_H