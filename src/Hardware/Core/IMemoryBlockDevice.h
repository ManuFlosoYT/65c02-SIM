#pragma once

#include <cstddef>
#include <cstdint>

namespace Hardware {

using Byte = uint8_t;

class IMemoryBlockDevice {
   public:
    virtual ~IMemoryBlockDevice() = default;
    [[nodiscard]] virtual Byte* GetRawMemory() = 0;
    [[nodiscard]] virtual size_t GetRawMemorySize() const = 0;
    [[nodiscard]] virtual bool IsReadOnly() const = 0;
};

}  // namespace Hardware
