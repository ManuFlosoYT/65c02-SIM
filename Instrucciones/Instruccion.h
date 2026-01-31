#ifndef MIPS_SIM_65C02_INSTRUCCION_H
#define MIPS_SIM_65C02_INSTRUCCION_H

class CPU;
class Mem;

class Instruccion {
    public:
        virtual ~Instruccion() = default;
        virtual void Ejecutar( CPU& cpu, Mem& mem ) const = 0;
};

#endif //MIPS_SIM_65C02_INSTRUCCION_H