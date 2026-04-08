#include "Hardware/Memory/ROM.h"

#include <iostream>

namespace Hardware {

ROM::ROM(size_t size) : size(size) { data.resize(size, 0); }

Byte ROM::Read(Word address) {
    if (address < size) {
        return data[address];
    }
    return 0;
}

void ROM::Write(Word address, Byte data) {
    // ROM is Read-Only via Bus
}

void ROM::Load(const std::vector<Byte>& buffer, Word offset) {
    for (size_t i = 0; i < buffer.size() && (offset + i) < size; ++i) {
        data[offset + i] = buffer[i];
    }
}

void ROM::WriteDirect(Word address, Byte data) {
    if (address < size) {
        this->data[address] = data;
    }
}

bool ROM::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, data);
    return out.good();
}

bool ROM::LoadState(std::istream& inputStream) {
    ISerializable::Deserialize(inputStream, data);
    return inputStream.good();
}

}  // namespace Hardware
