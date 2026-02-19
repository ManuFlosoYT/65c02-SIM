#include "LCD.h"

#include <cstring>

#include "Mem.h"

namespace Hardware {

void LCD::Init(Mem& mem) {
    // Reset state
    four_bit_mode = false;
    waiting_low_nibble = false;
    last_portb = 0;
    PORTB_DATA = 0;
    DDRB_DATA = 0;

    // Clear screen buffer
    std::memset(screen, ' ', sizeof(screen));
    cursorX = 0;
    cursorY = 0;

    is_init = false;

    mem.SetWriteHook(DDRB, [this](Word addr, Byte val) { DDRB_DATA = val; });
}

void LCD::Update(Byte val) {
    Byte old_val = last_portb;
    last_portb = val;
    PORTB_DATA = val;

    // Detect Falling Edge of E (Bit 6, 0x40)
    bool old_E = (old_val & 0x40) != 0;
    bool new_E = (val & 0x40) != 0;

    if (old_E && !new_E) {
        // E went High -> Low. Latch Data.
        Byte data_nibble = val & 0x0F;  // PB0-3 -> D4-D7
        bool rs = (val & 0x10) != 0;    // PB4 = RS
        bool rw = (val & 0x20) != 0;    // PB5 = RW

        // We only care about Writes (RW=0)
        if (rw) return;

        if (!four_bit_mode) {
            // 8-bit mode detection
            if (!rs && data_nibble == 0x02) {
                four_bit_mode = true;
                waiting_low_nibble = false;
                is_init = true;
            }
        } else {
            // 4-bit mode
            if (!waiting_low_nibble) {
                // Capture High Nibble
                current_high_nibble = data_nibble;
                waiting_low_nibble = true;
            } else {
                // Capture Low Nibble and Execute
                Byte low_nibble = data_nibble;
                Byte full_byte = (current_high_nibble << 4) | low_nibble;
                waiting_low_nibble = false;

                if (rs) {
                    // Data (Character)
                    WriteCharToScreen((char)full_byte);
                } else {
                    // Command
                    HandleCommand(full_byte);
                }
            }
        }
    }
}

void LCD::WriteCharToScreen(char c) {
    if (onChar) {
        if (c == '\n') onChar('\r');  // Convert \n to \r\n for terminal
        onChar(c);
        if (c == '\n')
            return;  // Don't print newline to LCD buffer directly as character
    }

    if (c == '\n' || c == '\r') {
        cursorX = 0;
        cursorY++;
        if (cursorY >= 2) cursorY = 0;  // Wrap to top
        return;
    }

    if (cursorX < 16 && cursorY < 2) {
        screen[cursorY][cursorX] = c;
    }

    cursorX++;
    if (cursorX >= 16) {
        cursorX = 0;
        cursorY++;
        if (cursorY >= 2) cursorY = 0;
    }
}

void LCD::HandleCommand(Byte cmd) {
    if (cmd == 0x01) {
        // Clear Display
        std::memset(screen, ' ', sizeof(screen));
        cursorX = 0;
        cursorY = 0;
    }
    // Other commands could be implemented here
}

}  // namespace Hardware
