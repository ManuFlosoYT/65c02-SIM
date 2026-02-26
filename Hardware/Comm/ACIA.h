#pragma once

#include <functional>
#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class ACIA : public IBusDevice {
public:
    ACIA();
    void Reset() override;
    void ReceiveData(Byte data);

    // IBusDevice implementation
    inline Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    inline std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    void SetOutputCallback(std::function<void(char)> cb);

    inline bool HasIRQ() const { return (STATUS & 0x80) != 0; }

private:
    Byte DATA;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    std::function<void(char)> outputCallback;
};

}  // namespace Hardware


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
