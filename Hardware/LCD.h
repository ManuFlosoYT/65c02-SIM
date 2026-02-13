#ifndef SIM_65C02_LCD_H
#define SIM_65C02_LCD_H

#include <functional>

#include "Mem.h"

class LCD {
public:
    Byte PORTB_DATA;
    Byte DDRB_DATA;

    void Init(Mem& mem);
    void Update(Byte portBVal);

    void SetOutputCallback(std::function<void(char)> cb) { onChar = cb; }
    const char (&GetScreen() const)[2][16] { return screen; }

private:
    bool four_bit_mode = false;
    bool waiting_low_nibble = false;
    Byte current_high_nibble = 0;
    Byte last_portb = 0;

    // Display State
    char screen[2][16];
    int cursorX = 0;
    int cursorY = 0;
    std::function<void(char)> onChar;

    void WriteCharToScreen(char c);
    void HandleCommand(Byte cmd);
};

#endif  // SIM_65C02_LCD_H