#pragma once

#include <functional>
#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class ACIA : public IBusDevice {
public:
    ACIA();
    void Reset() override;

    // IBusDevice implementation
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    std::string GetName() const override { return "ACIA"; }

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

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
