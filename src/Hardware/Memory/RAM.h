#pragma once
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class RAM : public IBusDevice {
   public:
    RAM(size_t size = 0x8000);
    ~RAM() override = default;
    RAM(const RAM&) = delete;
    RAM& operator=(const RAM&) = delete;
    RAM(RAM&&) = delete;
    RAM& operator=(RAM&&) = delete;

    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override;
    void Reset() override;
    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    Byte* GetRawMemory() override { return data.data(); }

   private:
    std::vector<Byte> data;
    size_t size;
};

}  // namespace Hardware

inline std::string Hardware::RAM::GetName() const { return "RAM"; }
