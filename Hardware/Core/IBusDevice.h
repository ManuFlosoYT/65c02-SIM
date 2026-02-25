#pragma once
#include <cstdint>
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
};

}  // namespace Hardware
