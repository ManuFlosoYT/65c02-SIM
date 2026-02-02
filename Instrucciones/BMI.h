#ifndef SIM_65C02_BMI_H
#define SIM_65C02_BMI_H

#include "../Componentes/CPU.h"
#include "../Componentes/Mem.h"
class BMI {
public:
    static void Ejecutar(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BMI_H