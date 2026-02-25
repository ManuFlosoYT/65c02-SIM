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
    std::string GetName() const override { return "RAM"; }
    void Reset() override;
    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

private:
    std::vector<Byte> data;
    size_t size;
};

}  // namespace Hardware
