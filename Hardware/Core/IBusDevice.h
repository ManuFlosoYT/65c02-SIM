#pragma once
#include <cstdint>
#include <iostream>
#include <string>

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

class IBusDevice {
public:
    virtual ~IBusDevice() = default;

    virtual Byte Read(Word address) = 0;
    virtual void Write(Word address, Byte data) = 0;

    virtual std::string GetName() const = 0;

    virtual void Reset() {}

    virtual bool SaveState(std::ostream& out) const = 0;
    virtual bool LoadState(std::istream& in) = 0;
};

}  // namespace Hardware
