#include "Hardware/Comm/ACIA.h"

#include "Hardware/Core/Bus.h"

namespace Hardware {

ACIA::ACIA() : DATA(0), STATUS(0), CMD(0), CTRL(0) { Reset(); }

void ACIA::Reset() {
    DATA = 0;
    STATUS = 0;
    CMD = 0;
    CTRL = 0;
}

void ACIA::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            DATA = data;
            if (outputCallback) {
                outputCallback((char)data);
            }
            break;
        case ACIA_STATUS & 0x03:
            STATUS = data;
            break;
        case ACIA_CMD & 0x03:
            CMD = data;
            break;
        case ACIA_CTRL & 0x03:
            CTRL = data;
            break;
        default:
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

bool ACIA::LoadState(std::istream& inStream) {
    inStream.read(reinterpret_cast<char*>(&DATA), sizeof(DATA));
    inStream.read(reinterpret_cast<char*>(&STATUS), sizeof(STATUS));
    inStream.read(reinterpret_cast<char*>(&CMD), sizeof(CMD));
    inStream.read(reinterpret_cast<char*>(&CTRL), sizeof(CTRL));
    return inStream.good();
}

}  // namespace Hardware
