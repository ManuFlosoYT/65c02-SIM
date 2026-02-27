#pragma once
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class ROM : public IBusDevice {
   public:
    ROM(size_t size = 0x8000);
    ~ROM() override = default;
    ROM(const ROM&) = delete;
    ROM& operator=(const ROM&) = delete;
    ROM(ROM&&) = delete;
    ROM& operator=(ROM&&) = delete;

    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override;
    void Reset() override;
    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    Byte* GetRawMemory() override { return data.data(); }
    [[nodiscard]] bool IsReadOnly() const override { return true; }

    void Load(const std::vector<Byte>& buffer, Word offset = 0);
    void WriteDirect(Word address, Byte data);

   private:
    std::vector<Byte> data;
    size_t size;
};

}  // namespace Hardware

inline std::string Hardware::ROM::GetName() const { return "ROM"; }
inline void Hardware::ROM::Reset() {}
