#include "Hardware/Video/LCD.h"
#include "Hardware/Core/Bus.h"

namespace Hardware {

LCD::LCD() : screen{} { Reset(); }

void LCD::Reset() {
    busLatch.Reset();

    for (auto& row : screen) {
        row.fill(' ');
    }
    cursorX = 0;
    cursorY = 0;
    display_on = true;
    cursor_on = false;
    cursor_increment = true;
}

Byte LCD::Read(Word address) {
    if (address == PORTB) {
        return busLatch.PORTB_DATA;
    }
    return 0;
}

void LCD::Write(Word address, Byte data) {
    if (address == PORTB) {
        Update(data);
    }
}

void LCD::Update(Byte val) {
    if (auto transfer = busLatch.ProcessPortB(val)) {
        if (transfer.is_data) {
            WriteCharToScreen(static_cast<char>(transfer.value));
        } else {
            HandleCommand(transfer.value);
        }
    }
}

void LCD::WriteCharToScreen(char character) {
    if (onChar) {
        if (character == '\n') {
            onChar('\r');  // Convert \n to \r\n for terminal
        }
        onChar(character);
        if (character == '\n') {
            return;  // Don't print newline to LCD buffer directly as character
        }
    }

    if (character == '\n' || character == '\r') {
        cursorX = 0;
        cursorY++;
        if (cursorY >= 2) {
            cursorY = 0;  // Wrap to top
        }
        return;
    }

    if (cursorX >= 0 && cursorX < 16 && cursorY >= 0 && cursorY < 2) {
        screen.at(cursorY).at(cursorX) = character;
    }

    if (cursor_increment) {
        cursorX++;
        if (cursorX >= 16) {
            cursorX = 0;
            cursorY++;
            if (cursorY >= 2) {
                cursorY = 0;
            }
        }
    } else {
        cursorX--;
        if (cursorX < 0) {
            cursorX = 15;
            cursorY--;
            if (cursorY < 0) {
                cursorY = 1;
            }
        }
    }
}

void LCD::HandleCommand(Byte cmd) {
    switch (cmd) {
        case 0x01:
            for (auto& row : screen) {
                row.fill(' ');
            }
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
            busLatch.four_bit_mode = true;
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
                cursorX = std::min(cursorX, 15);
            }
            break;
    }
}

bool LCD::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, busLatch.PORTB_DATA);
    ISerializable::Serialize(out, busLatch.four_bit_mode);
    ISerializable::Serialize(out, busLatch.waiting_low_nibble);
    ISerializable::Serialize(out, busLatch.current_high_nibble);
    ISerializable::Serialize(out, busLatch.last_portb);
    ISerializable::Serialize(out, busLatch.is_init);
    ISerializable::Serialize(out, screen);
    ISerializable::Serialize(out, cursorX);
    ISerializable::Serialize(out, cursorY);
    ISerializable::Serialize(out, display_on);
    ISerializable::Serialize(out, cursor_on);
    ISerializable::Serialize(out, cursor_increment);
    return out.good();
}

bool LCD::LoadState(std::istream& inputStream) {
    ISerializable::Deserialize(inputStream, busLatch.PORTB_DATA);
    ISerializable::Deserialize(inputStream, busLatch.four_bit_mode);
    ISerializable::Deserialize(inputStream, busLatch.waiting_low_nibble);
    ISerializable::Deserialize(inputStream, busLatch.current_high_nibble);
    ISerializable::Deserialize(inputStream, busLatch.last_portb);
    ISerializable::Deserialize(inputStream, busLatch.is_init);
    ISerializable::Deserialize(inputStream, screen);
    ISerializable::Deserialize(inputStream, cursorX);
    ISerializable::Deserialize(inputStream, cursorY);
    ISerializable::Deserialize(inputStream, display_on);
    ISerializable::Deserialize(inputStream, cursor_on);
    ISerializable::Deserialize(inputStream, cursor_increment);
    return inputStream.good();
}

}  // namespace Hardware
