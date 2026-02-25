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

void ROM::Write(Word address, Byte value) {
    // ROM is Read-Only via Bus
}

void ROM::Load(const std::vector<Byte>& buffer, Word offset) {
    for (size_t i = 0; i < buffer.size() && (offset + i) < size; ++i) {
        data[offset + i] = buffer[i];
    }
}

void ROM::WriteDirect(Word address, Byte value) {
    if (address < size) {
        data[address] = value;
    }
}

bool ROM::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

bool ROM::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(data.data()), data.size());
    return in.good();
}

}  // namespace Hardware
