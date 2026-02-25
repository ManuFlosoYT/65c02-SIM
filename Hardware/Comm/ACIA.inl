// ACIA.inl — Inline implementations for Hardware::ACIA

#include "Hardware/Core/Bus.h"

namespace Hardware {

inline std::string ACIA::GetName() const { return "ACIA"; }

inline void ACIA::SetOutputCallback(std::function<void(char)> cb) {
    outputCallback = cb;
}

inline Byte ACIA::Read(Word address) {
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

}  // namespace Hardware
