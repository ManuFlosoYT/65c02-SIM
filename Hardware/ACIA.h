#ifndef SIM_65C02_ACIA_H
#define SIM_65C02_ACIA_H

#include <functional>

#include "Mem.h"

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

#endif  // SIM_65C02_ACIA_H