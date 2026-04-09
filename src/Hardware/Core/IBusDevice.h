#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include "Hardware/Core/ISerializable.h"

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

class IBusDevice : public ISerializable {
   public:
    IBusDevice() = default;
    ~IBusDevice() override = default;
    IBusDevice(const IBusDevice&) = delete;
    IBusDevice& operator=(const IBusDevice&) = delete;
    IBusDevice(IBusDevice&&) = delete;
    IBusDevice& operator=(IBusDevice&&) = delete;

    virtual Byte Read(Word address) = 0;
    virtual void Write(Word address, Byte data) = 0;
    virtual void WriteDirect(Word address, Byte data) { Write(address, data); }

    [[nodiscard]] virtual std::string GetName() const = 0;

    virtual void Reset();

    bool SaveState(std::ostream& out) const override = 0;
    bool LoadState(std::istream& inStream) override = 0;

    // Optional direct memory access for cache optimization (returns nullptr if not supported)
    [[nodiscard]] virtual Byte* GetRawMemory() { return nullptr; }
    [[nodiscard]] virtual bool IsReadOnly() const { return false; }
};

}  // namespace Hardware

inline void Hardware::IBusDevice::Reset() {}
