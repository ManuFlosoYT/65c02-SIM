#include "LCD.h"

#include <iostream>

#include "Mem.h"

void LCD::init(Mem& mem) {
    PORTB_DATA = mem[PORTB];
    DDRB_DATA = mem[DDRB];

    mem.SetWriteHook([this](Word addr, Byte val) {
        if (addr == PORTB) {
            std::cout << (char)val;
            std::cout.flush();
        }
    });
}