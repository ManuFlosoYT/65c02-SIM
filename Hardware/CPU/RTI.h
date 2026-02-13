#ifndef SIM_65C02_RTI_H
#define SIM_65C02_RTI_H

#include "../CPU.h"
#include "../Mem.h"

class RTI {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_RTI_H
