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
    ISerializable::Serialize(out, DATA);
    ISerializable::Serialize(out, STATUS);
    ISerializable::Serialize(out, CMD);
    ISerializable::Serialize(out, CTRL);
    return out.good();
}

bool ACIA::LoadState(std::istream& inStream) {
    ISerializable::Deserialize(inStream, DATA);
    ISerializable::Deserialize(inStream, STATUS);
    ISerializable::Deserialize(inStream, CMD);
    ISerializable::Deserialize(inStream, CTRL);
    return inStream.good();
}

}  // namespace Hardware
