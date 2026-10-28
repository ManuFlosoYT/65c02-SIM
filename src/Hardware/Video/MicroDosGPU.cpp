#include "Hardware/Video/MicroDosGPU.h"

#include <algorithm>

namespace Hardware {

void MicroDosGPU::Reset() {
    GPU::Reset();
    extraRam.fill(0);
}

void MicroDosGPU::LoadVRAM(std::span<const uint8_t> data) {
    GPU::LoadVRAM(data);
    size_t limit = std::min(data.size(), MEMORY_SIZE);
    for (size_t i = 0; i < limit; ++i) {
        Byte pixelXAddr = static_cast<Byte>(i & 0x7F);
        Byte pixelYAddr = static_cast<Byte>((i >> 7) & 0x7F);
        if (pixelXAddr >= VRAM_WIDTH || pixelYAddr >= VRAM_HEIGHT) {
            extraRam[i] = data[i];
        }
    }
}

bool MicroDosGPU::SaveState(std::ostream& out) const {
    if (!GPU::SaveState(out)) {
        return false;
    }
    ISerializable::Serialize(out, extraRam);
    return out.good();
}

bool MicroDosGPU::LoadState(std::istream& inputStream) {
    if (!GPU::LoadState(inputStream)) {
        return false;
    }
    ISerializable::Deserialize(inputStream, extraRam);
    return inputStream.good();
}

}  // namespace Hardware

