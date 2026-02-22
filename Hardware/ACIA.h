#pragma once

#include <functional>
#include <iostream>

#include "Mem.h"

namespace Hardware {

class ACIA {
public:
    Byte DATA;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    void Init(Mem& mem);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    void SetOutputCallback(std::function<void(char)> cb) {
        outputCallback = cb;
    }

private:
    std::function<void(char)> outputCallback;
};

}  // namespace Hardware
