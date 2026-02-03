#include "LCD.h"

#include <iostream>

#include "Mem.h"

void LCD::init(Mem& mem) {
    PORTB_DATA = mem[PORTB];
    DDRB_DATA = mem[DDRB];
}

void LCD::update(Mem& mem) {
    std::cout << (char)mem[PORTB];
    std::cout.flush();
}