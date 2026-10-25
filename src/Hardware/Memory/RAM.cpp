#include "Hardware/Memory/RAM.h"

#include <algorithm>
#include <random>

namespace Hardware {

RAM::RAM(size_t size) { data.resize(size, 0); }

Byte RAM::Read(Word address) {
    if (address < data.size()) {
        return data[address];
    }
    return 0;
}

void RAM::Write(Word address, Byte data) {
    if (address < this->data.size()) {
        this->data[address] = data;
    }
}

void RAM::Reset() {
    if (!powerOnPattern) {
        std::ranges::fill(data, 0);
        return;
    }
    std::mt19937 rng{std::random_device{}()};
    std::ranges::generate(data, [&] { return static_cast<Byte>(rng()); });
}

bool RAM::SaveState(std::ostream& out) const {
    auto sz = static_cast<uint32_t>(data.size());
    ISerializable::Serialize(out, sz);
    ISerializable::Serialize(out, data);
    return out.good();
}

bool RAM::LoadState(std::istream& inputStream) {
    uint32_t sz = 0;
    ISerializable::Deserialize(inputStream, sz);
    if (sz <= 0x10000 && sz > 0) {
        data.resize(sz);
    }
    ISerializable::Deserialize(inputStream, data);
    return inputStream.good();
}

}  // namespace Hardware
