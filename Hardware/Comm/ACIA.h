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

#include "Hardware/Comm/ACIA.inl"
