#include "Hardware/Comm/ACIA.h"

#include "Hardware/Core/Bus.h"

namespace Hardware {

ACIA::ACIA() : txData(0), rxData(0), STATUS(0), CMD(0), CTRL(0) { Reset(); }

void ACIA::UpdateIRQ() {
    bool irq = false;
    if ((STATUS & ACIA_STATUS_RDRF) != 0 && (CMD & ACIA_CMD_RX_IRQ_DISABLE) == 0) {
        irq = true;
    }
    if ((STATUS & ACIA_STATUS_TDRE) != 0 && (CMD & 0x0C) == 0x04) {
        irq = true;
    }

    if (irq) {
        STATUS |= ACIA_STATUS_IRQ;
    } else {
        STATUS &= ~ACIA_STATUS_IRQ;
    }
}

void ACIA::Reset() {
    txData = 0;
    rxData = 0;
    STATUS = ACIA_STATUS_TDRE;
    CMD = 0;
    CTRL = 0;
}

void ACIA::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            txData = data;
            STATUS &= ~ACIA_STATUS_TDRE;
            if (outputCallback) {
                outputCallback((char)data);
            }
            STATUS |= ACIA_STATUS_TDRE;
            UpdateIRQ();
            break;
        case ACIA_STATUS & 0x03:
            STATUS &= ~(ACIA_STATUS_IRQ | ACIA_STATUS_OVRN | ACIA_STATUS_PE | ACIA_STATUS_FE | ACIA_STATUS_RDRF);
            STATUS |= ACIA_STATUS_TDRE;
            UpdateIRQ();
            break;
        case ACIA_CMD & 0x03:
            CMD = data;
            UpdateIRQ();
            break;
        case ACIA_CTRL & 0x03:
            CTRL = data;
            break;
        default:
            break;
    }
}

void ACIA::ReceiveData(Byte data) {
    if ((CMD & ACIA_CMD_DTR) == 0) return;

    if ((STATUS & ACIA_STATUS_RDRF) != 0) {
        STATUS |= ACIA_STATUS_OVRN;
    }
    rxData = data;
    STATUS |= ACIA_STATUS_RDRF;

    if ((CMD & ACIA_CMD_ECHO) != 0) {
        if (outputCallback) {
            outputCallback((char)data);
        }
    }
    UpdateIRQ();
}

bool ACIA::CanReceive() const {
    return ((CMD & ACIA_CMD_DTR) != 0) && ((STATUS & ACIA_STATUS_RDRF) == 0);
}

int ACIA::GetBaudRate() const {
    static const int baudRates[16] = {
        115200, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
    };
    return baudRates[CTRL & 0x0F];
}

bool ACIA::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, txData);
    ISerializable::Serialize(out, rxData);
    ISerializable::Serialize(out, STATUS);
    ISerializable::Serialize(out, CMD);
    ISerializable::Serialize(out, CTRL);
    return out.good();
}

bool ACIA::LoadState(std::istream& inStream) {
    ISerializable::Deserialize(inStream, txData);
    ISerializable::Deserialize(inStream, rxData);
    ISerializable::Deserialize(inStream, STATUS);
    ISerializable::Deserialize(inStream, CMD);
    ISerializable::Deserialize(inStream, CTRL);
    return inStream.good();
}

}  // namespace Hardware
