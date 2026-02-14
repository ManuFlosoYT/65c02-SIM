#pragma once

#include <functional>

#include "Mem.h"

namespace Hardware {

class ACIA {
public:
    Byte DATA;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    void Init(Mem& mem);

    void SetOutputCallback(std::function<void(char)> cb) {
        outputCallback = cb;
    }

private:
    std::function<void(char)> outputCallback;
};

}  // namespace Hardware
