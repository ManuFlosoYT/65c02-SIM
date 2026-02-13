#ifndef SIM_65C02_BMI_H
#define SIM_65C02_BMI_H

#include "../CPU.h"
#include "../Mem.h"
class BMI {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_BMI_H