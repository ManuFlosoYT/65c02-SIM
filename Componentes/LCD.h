#ifndef SIM_65C02_LCD_H
#define SIM_65C02_LCD_H

#include "Mem.h"

class LCD {
public:
    Byte DATO_PORTB;
    Byte DATO_DDRB;

    void Inicializar(Mem& mem);

private:
    bool modo_cuatro_bits = false;
    bool esperando_nibble_bajo = false;
    Byte nibble_alto_actual = 0;
    Byte ultimo_portb = 0;
};

#endif  // SIM_65C02_LCD_H