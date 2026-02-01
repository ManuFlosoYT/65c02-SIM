#ifndef MIPS_SIM_65C02_JMP_H
#define MIPS_SIM_65C02_JMP_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"


class JMP {
    public:
        static void EjecutarABS( CPU& cpu, Mem& mem );
        static void EjecutarIND( CPU& cpu, Mem& mem);
        static void EjecutarABSX( CPU& cpu, Mem& mem );
};

#endif //MIPS_SIM_65C02_JMP_H