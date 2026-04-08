#include "Hardware/Memory/RAM.h"

#include <algorithm>
#include <iostream>

namespace Hardware {

RAM::RAM(size_t size) : size(size) { data.resize(size, 0); }

Byte RAM::Read(Word address) {
    if (address < size) {
        return data[address];
    }
    return 0;
}

void RAM::Write(Word address, Byte data) {
    if (address < size) {
        this->data[address] = data;
    }
}

void RAM::Reset() { std::ranges::fill(data, 0); }

bool RAM::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, data);
    return out.good();
}

bool RAM::LoadState(std::istream& inputStream) {
    ISerializable::Deserialize(inputStream, data);
    return inputStream.good();
}

}  // namespace Hardware
