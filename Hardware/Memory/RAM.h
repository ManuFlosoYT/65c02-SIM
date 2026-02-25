#pragma once
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class RAM : public IBusDevice {
public:
    RAM(size_t size = 0x8000);
    ~RAM() override = default;

    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    std::string GetName() const override;
    void Reset() override;
    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    Byte* GetRawMemory() override { return data.data(); }

private:
    std::vector<Byte> data;
    size_t size;
};

}  // namespace Hardware

#include "Hardware/Memory/RAM.inl"
