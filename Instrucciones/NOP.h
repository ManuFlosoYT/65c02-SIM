//
// Created by manu on 29/1/26.
//

#ifndef MIPS_SIM_65C02_NOP_H
#define MIPS_SIM_65C02_NOP_H
#include "Instruccion.h"


class NOP : public Instruccion {
    public:
        void Ejecutar() const override;
};

#endif //MIPS_SIM_65C02_NOP_H