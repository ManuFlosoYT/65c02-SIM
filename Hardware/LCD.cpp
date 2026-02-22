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

    mem.SetWriteHook(
        DDRB,
        [](void* context, Word addr, Byte val) {
            static_cast<LCD*>(context)->DDRB_DATA = val;
        },
        this);
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

    if (cursor_increment) {
        cursorX++;
        if (cursorX >= 16) {
            cursorX = 0;
            cursorY++;
            if (cursorY >= 2) cursorY = 0;
        }
    } else {
        cursorX--;
        if (cursorX < 0) {
            cursorX = 15;
            cursorY--;
            if (cursorY < 0) cursorY = 1;
        }
    }
}

void LCD::HandleCommand(Byte cmd) {
    switch (cmd) {
        case 0x01:
            std::memset(screen, ' ', sizeof(screen));
            cursorX = 0;
            cursorY = 0;
            break;
        case 0x02:
            cursorX = 0;
            cursorY = 0;
            break;
        case 0x06:
            cursor_increment = true;
            break;
        case 0x0C:
            display_on = true;
            cursor_on = false;
            break;
        case 0x0E:
            display_on = true;
            cursor_on = true;
            break;
        case 0x28:
            four_bit_mode = true;
            break;
        case 0x80:
            cursorX = 0;
            cursorY = 0;
            break;
        case 0xC0:
            cursorX = 0;
            cursorY = 1;
            break;
        default:
            if ((cmd & 0x80) == 0x80) {
                Byte addr = cmd & 0x7F;
                if (addr >= 0x40) {
                    cursorY = 1;
                    cursorX = addr - 0x40;
                } else {
                    cursorY = 0;
                    cursorX = addr;
                }
                if (cursorX > 15) cursorX = 15;
            }
            break;
    }
}

bool LCD::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&PORTB_DATA), sizeof(PORTB_DATA));
    out.write(reinterpret_cast<const char*>(&DDRB_DATA), sizeof(DDRB_DATA));
    out.write(reinterpret_cast<const char*>(&four_bit_mode),
              sizeof(four_bit_mode));
    out.write(reinterpret_cast<const char*>(&waiting_low_nibble),
              sizeof(waiting_low_nibble));
    out.write(reinterpret_cast<const char*>(&current_high_nibble),
              sizeof(current_high_nibble));
    out.write(reinterpret_cast<const char*>(&last_portb), sizeof(last_portb));
    out.write(reinterpret_cast<const char*>(&is_init), sizeof(is_init));
    out.write(reinterpret_cast<const char*>(screen), sizeof(screen));
    out.write(reinterpret_cast<const char*>(&cursorX), sizeof(cursorX));
    out.write(reinterpret_cast<const char*>(&cursorY), sizeof(cursorY));
    out.write(reinterpret_cast<const char*>(&display_on), sizeof(display_on));
    out.write(reinterpret_cast<const char*>(&cursor_on), sizeof(cursor_on));
    out.write(reinterpret_cast<const char*>(&cursor_increment),
              sizeof(cursor_increment));
    return out.good();
}

bool LCD::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&PORTB_DATA), sizeof(PORTB_DATA));
    in.read(reinterpret_cast<char*>(&DDRB_DATA), sizeof(DDRB_DATA));
    in.read(reinterpret_cast<char*>(&four_bit_mode), sizeof(four_bit_mode));
    in.read(reinterpret_cast<char*>(&waiting_low_nibble),
            sizeof(waiting_low_nibble));
    in.read(reinterpret_cast<char*>(&current_high_nibble),
            sizeof(current_high_nibble));
    in.read(reinterpret_cast<char*>(&last_portb), sizeof(last_portb));
    in.read(reinterpret_cast<char*>(&is_init), sizeof(is_init));
    in.read(reinterpret_cast<char*>(screen), sizeof(screen));
    in.read(reinterpret_cast<char*>(&cursorX), sizeof(cursorX));
    in.read(reinterpret_cast<char*>(&cursorY), sizeof(cursorY));
    in.read(reinterpret_cast<char*>(&display_on), sizeof(display_on));
    in.read(reinterpret_cast<char*>(&cursor_on), sizeof(cursor_on));
    in.read(reinterpret_cast<char*>(&cursor_increment),
            sizeof(cursor_increment));
    return in.good();
}

}  // namespace Hardware
