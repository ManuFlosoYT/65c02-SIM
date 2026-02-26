#pragma once
#include <cstdint>
#include <iostream>
#include <string>

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

class IBusDevice {
   public:
    IBusDevice() = default;
    virtual ~IBusDevice() = default;
    IBusDevice(const IBusDevice&) = delete;
    IBusDevice& operator=(const IBusDevice&) = delete;
    IBusDevice(IBusDevice&&) = delete;
    IBusDevice& operator=(IBusDevice&&) = delete;

    virtual Byte Read(Word address) = 0;
    virtual void Write(Word address, Byte data) = 0;

    [[nodiscard]] virtual std::string GetName() const = 0;

    virtual void Reset();

    virtual bool SaveState(std::ostream& out) const = 0;
    virtual bool LoadState(std::istream& inStream) = 0;

    // Optional direct memory access for cache optimization (returns nullptr if not supported)
    [[nodiscard]] virtual Byte* GetRawMemory() { return nullptr; }
    [[nodiscard]] virtual bool IsReadOnly() const { return false; }
};

}  // namespace Hardware

inline void Hardware::IBusDevice::Reset() {}
