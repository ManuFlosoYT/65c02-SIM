#pragma once
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class ROM : public IBusDevice {
public:
    ROM(size_t size = 0x8000);
    ~ROM() override = default;

    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    std::string GetName() const override;
    void Reset() override;
    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    void Load(const std::vector<Byte>& buffer, Word offset = 0);
    void WriteDirect(Word address, Byte data);

private:
    std::vector<Byte> data;
    size_t size;
};

}  // namespace Hardware

#include "Hardware/Memory/ROM.inl"
