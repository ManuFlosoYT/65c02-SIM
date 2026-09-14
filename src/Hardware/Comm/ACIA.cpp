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
    if (dcdInterrupt || dsrInterrupt) {
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
    
    shiftRegisterTicks = 0;
    shiftRegisterData = 0;
    isShifting = false;
    echoPending = false;
    echoByte = 0;

    dcdState = false;
    dsrState = false;
    dcdInterrupt = false;
    dsrInterrupt = false;
}

void ACIA::SetDCD(bool active) {
    bool prevState = dcdState;
    dcdState = active;
    if (active) {
        STATUS &= ~ACIA_STATUS_DCD; // Carrier present (active low pin)
    } else {
        STATUS |= ACIA_STATUS_DCD;  // Carrier lost
        if (prevState) dcdInterrupt = true;
    }
    UpdateIRQ();
}

void ACIA::SetDSR(bool active) {
    bool prevState = dsrState;
    dsrState = active;
    if (active) {
        STATUS &= ~ACIA_STATUS_DSR;
    } else {
        STATUS |= ACIA_STATUS_DSR;
        if (prevState) dsrInterrupt = true;
    }
    UpdateIRQ();
}

void ACIA::Clock(int ips) {
    if (isShifting) {
        if (shiftRegisterTicks > 0) {
            shiftRegisterTicks--;
            if (shiftRegisterTicks == 0) {
                if (outputCallback) {
                    outputCallback((char)shiftRegisterData);
                }
                isShifting = false;
            }
        }
    }

    if (!isShifting) {
        // Echo has highest priority for the TSR
        if (echoPending) {
            shiftRegisterData = echoByte;
            echoPending = false;
            isShifting = true;
            
            int baud = GetBaudRate();
            if (baud <= 0) baud = 115200;
            shiftRegisterTicks = (ips * GetFrameBits()) / baud;
            if (shiftRegisterTicks <= 0) shiftRegisterTicks = 1;
        } 
        // If transmitter is on and not in echo mode, check TDRE
        else if ((CMD & ACIA_CMD_ECHO) == 0 && (CMD & ACIA_CMD_DTR) != 0) {
            if ((STATUS & ACIA_STATUS_TDRE) == 0) {
                shiftRegisterData = txData;
                isShifting = true;
                
                int baud = GetBaudRate();
                if (baud <= 0) baud = 115200;
                shiftRegisterTicks = (ips * GetFrameBits()) / baud;
                if (shiftRegisterTicks <= 0) shiftRegisterTicks = 1;

                STATUS |= ACIA_STATUS_TDRE;
                UpdateIRQ();
            }
        }
    }
}

void ACIA::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            txData = data;
            STATUS &= ~ACIA_STATUS_TDRE;
            UpdateIRQ();
            break;
        case ACIA_STATUS & 0x03:
            // Programmed reset
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

    if ((CMD & ACIA_CMD_ECHO) != 0) {
        echoPending = true;
        echoByte = data;
    }

    if ((STATUS & ACIA_STATUS_RDRF) != 0) {
        STATUS |= ACIA_STATUS_OVRN;
    }
    rxData = data;
    STATUS |= ACIA_STATUS_RDRF;

    UpdateIRQ();
}

bool ACIA::CanReceive() const {
    return ((CMD & ACIA_CMD_DTR) != 0) && ((STATUS & ACIA_STATUS_RDRF) == 0);
}

int ACIA::GetBaudRate() const {
    static const int baudRates[16] = {
        0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
    };
    return baudRates[CTRL & 0x0F];
}

int ACIA::GetFrameBits() const {
    int bits = 1; // Start bit

    // Word length
    Byte wl = (CTRL >> 5) & 0x03;
    switch (wl) {
        case 0: bits += 8; break;
        case 1: bits += 7; break;
        case 2: bits += 6; break;
        case 3: bits += 5; break;
    }

    // Parity
    if ((CMD & 0x20) != 0) {
        bits += 1;
    }

    // Stop bits
    if ((CTRL & 0x80) != 0) {
        bits += 2;
    } else {
        bits += 1;
    }

    return bits;
}

bool ACIA::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, txData);
    ISerializable::Serialize(out, rxData);
    ISerializable::Serialize(out, STATUS);
    ISerializable::Serialize(out, CMD);
    ISerializable::Serialize(out, CTRL);
    ISerializable::Serialize(out, shiftRegisterTicks);
    ISerializable::Serialize(out, shiftRegisterData);
    ISerializable::Serialize(out, isShifting);
    ISerializable::Serialize(out, echoPending);
    ISerializable::Serialize(out, echoByte);
    ISerializable::Serialize(out, dcdState);
    ISerializable::Serialize(out, dsrState);
    ISerializable::Serialize(out, dcdInterrupt);
    ISerializable::Serialize(out, dsrInterrupt);
    return out.good();
}

bool ACIA::LoadState(std::istream& inStream) {
    ISerializable::Deserialize(inStream, txData);
    ISerializable::Deserialize(inStream, rxData);
    ISerializable::Deserialize(inStream, STATUS);
    ISerializable::Deserialize(inStream, CMD);
    ISerializable::Deserialize(inStream, CTRL);
    ISerializable::Deserialize(inStream, shiftRegisterTicks);
    ISerializable::Deserialize(inStream, shiftRegisterData);
    ISerializable::Deserialize(inStream, isShifting);
    ISerializable::Deserialize(inStream, echoPending);
    ISerializable::Deserialize(inStream, echoByte);
    ISerializable::Deserialize(inStream, dcdState);
    ISerializable::Deserialize(inStream, dsrState);
    ISerializable::Deserialize(inStream, dcdInterrupt);
    ISerializable::Deserialize(inStream, dsrInterrupt);
    return inStream.good();
}

}  // namespace Hardware
