#include "Hardware/Memory/ROM.h"

namespace Hardware {

ROM::ROM(size_t size) { data.resize(size, 0); }

Byte ROM::Read(Word address) {
    if (address < data.size()) {
        return data[address];
    }
    return 0;
}

void ROM::Write(Word address, Byte data) {
    // ROM is Read-Only via Bus
}

void ROM::WriteDirect(Word address, Byte data) {
    if (address < this->data.size()) {
        this->data[address] = data;
    }
}

bool ROM::SaveState(std::ostream& out) const {
    auto sz = static_cast<uint32_t>(data.size());
    ISerializable::Serialize(out, sz);
    ISerializable::Serialize(out, data);
    return out.good();
}

bool ROM::LoadState(std::istream& inputStream) {
    uint32_t sz = 0;
    ISerializable::Deserialize(inputStream, sz);
    if (sz <= 0x10000 && sz > 0) {
        data.resize(sz);
    }
    ISerializable::Deserialize(inputStream, data);
    return inputStream.good();
}

}  // namespace Hardware
