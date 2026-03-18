#pragma once
#include <iostream>

namespace Hardware {

class ISerializable {
   public:
    ISerializable() = default;
    virtual ~ISerializable() = default;
    ISerializable(const ISerializable&) = delete;
    ISerializable& operator=(const ISerializable&) = delete;
    ISerializable(ISerializable&&) = delete;
    ISerializable& operator=(ISerializable&&) = delete;

    virtual bool SaveState(std::ostream& out) const = 0;
    virtual bool LoadState(std::istream& inStream) = 0;
};

}  // namespace Hardware
