//
// Created by manu on 29/1/26.
//

#ifndef MIPS_SIM_65C02_INSTRUCCION_H
#define MIPS_SIM_65C02_INSTRUCCION_H

class Instruccion {
    public:
        virtual ~Instruccion() = default;
        virtual void Ejecutar() const = 0;
};

#endif //MIPS_SIM_65C02_INSTRUCCION_H