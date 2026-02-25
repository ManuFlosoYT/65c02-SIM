#pragma once

#include <functional>
#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class LCD : public IBusDevice {
public:
    LCD();
    void Reset() override;

    // IBusDevice implementation
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    std::string GetName() const override { return "LCD"; }

    void Update(Byte portBVal);

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    void SetOutputCallback(std::function<void(char)> cb) { onChar = cb; }

    const char (&GetScreen() const)[2][16] { return screen; }

    bool IsInitialized() const { return is_init; }

    bool IsDisplayOn() const { return display_on; }
    bool IsCursorOn() const { return cursor_on; }
    int GetCursorX() const { return cursorX; }
    int GetCursorY() const { return cursorY; }

private:
    Byte PORTB_DATA;
    Byte DDRB_DATA;

    bool four_bit_mode = false;
    bool waiting_low_nibble = false;
    Byte current_high_nibble = 0;
    Byte last_portb = 0;

    bool is_init = false;

    // Display State
    char screen[2][16];
    int cursorX = 0;
    int cursorY = 0;

    bool display_on = true;
    bool cursor_on = false;
    bool cursor_increment = true;

    std::function<void(char)> onChar;

    void WriteCharToScreen(char c);
    void HandleCommand(Byte cmd);
};

}  // namespace Hardware
