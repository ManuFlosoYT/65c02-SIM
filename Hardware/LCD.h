#ifndef SIM_65C02_LCD_H
#define SIM_65C02_LCD_H

#include <functional>

#include "Mem.h"

class LCD {
public:
    Byte DATO_PORTB;
    Byte DATO_DDRB;

    void Inicializar(Mem& mem);
    void Update(Byte portBVal);

    void SetOutputCallback(std::function<void(char)> cb) { onChar = cb; }
    const char (&GetScreen() const)[2][16] { return screen; }

private:
    bool modo_cuatro_bits = false;
    bool esperando_nibble_bajo = false;
    Byte nibble_alto_actual = 0;
    Byte ultimo_portb = 0;

    // Display State
    char screen[2][16];
    int cursorX = 0;
    int cursorY = 0;
    std::function<void(char)> onChar;

    void WriteCharToScreen(char c);
    void HandleCommand(Byte cmd);
};

#endif  // SIM_65C02_LCD_H