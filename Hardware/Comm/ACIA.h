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
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    std::string GetName() const override { return "ACIA"; }

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    void SetOutputCallback(std::function<void(char)> cb) {
        outputCallback = cb;
    }

private:
    Byte DATA;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    std::function<void(char)> outputCallback;
};

}  // namespace Hardware
