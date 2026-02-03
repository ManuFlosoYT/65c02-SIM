#ifndef SIM_65C02_LCD_H
#define SIM_65C02_LCD_H

#include "Mem.h"

class LCD {
public:
    Byte PORTB_DATA;
    Byte DDRB_DATA;

    void init(Mem& mem);
    void update(Mem& mem);
};

#endif  // SIM_65C02_LCD_H