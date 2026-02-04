#ifndef SIM_65C02_ACIA_H
#define SIM_65C02_ACIA_H

#include "Mem.h"

class ACIA {
public:
    Byte DATO;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    void Inicializar(Mem& mem);
};

#endif  // SIM_65C02_ACIA_H