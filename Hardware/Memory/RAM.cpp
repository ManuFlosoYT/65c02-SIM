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

void RAM::Write(Word address, Byte value) {
    if (address < size) {
        data[address] = value;
    }
}

void RAM::Reset() { std::fill(data.begin(), data.end(), 0); }

bool RAM::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

bool RAM::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(data.data()), data.size());
    return in.good();
}

}  // namespace Hardware
