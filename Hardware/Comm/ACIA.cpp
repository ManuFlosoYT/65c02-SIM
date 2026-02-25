#include "Hardware/Comm/ACIA.h"

#include "Hardware/Core/Bus.h"

namespace Hardware {

ACIA::ACIA() { Reset(); }

void ACIA::Reset() {
    DATA = 0;
    STATUS = 0;
    CMD = 0;
    CTRL = 0;
}

Byte ACIA::Read(Word address) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            STATUS &= ~0x80;  // Clear interrupt/ready bit on read
            return DATA;
        case ACIA_STATUS & 0x03:
            return STATUS;
        case ACIA_CMD & 0x03:
            return CMD;
        case ACIA_CTRL & 0x03:
            return CTRL;
        default:
            return 0;
    }
}

void ACIA::Write(Word address, Byte val) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            DATA = val;
            if (outputCallback) {
                outputCallback((char)val);
            }
            break;
        case ACIA_STATUS & 0x03:
            STATUS = val;
            break;
        case ACIA_CMD & 0x03:
            CMD = val;
            break;
        case ACIA_CTRL & 0x03:
            CTRL = val;
            break;
    }
}

void ACIA::ReceiveData(Byte data) {
    DATA = data;
    STATUS |= 0x80;
}

bool ACIA::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&DATA), sizeof(DATA));
    out.write(reinterpret_cast<const char*>(&STATUS), sizeof(STATUS));
    out.write(reinterpret_cast<const char*>(&CMD), sizeof(CMD));
    out.write(reinterpret_cast<const char*>(&CTRL), sizeof(CTRL));
    return out.good();
}

bool ACIA::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&DATA), sizeof(DATA));
    in.read(reinterpret_cast<char*>(&STATUS), sizeof(STATUS));
    in.read(reinterpret_cast<char*>(&CMD), sizeof(CMD));
    in.read(reinterpret_cast<char*>(&CTRL), sizeof(CTRL));
    return in.good();
}

}  // namespace Hardware
