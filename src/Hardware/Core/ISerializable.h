#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "Hardware/Core/BinarySerializer.h"

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

    template <typename T>
    static void Serialize(std::ostream& out, const T& value) {
        BinarySerializer::Serialize(out, value);
    }

    template <typename T>
    static void Serialize(std::ostream& out, const std::vector<T>& vec) {
        BinarySerializer::Serialize(out, vec);
    }

    static void Serialize(std::ostream& out, const std::string& str) {
        BinarySerializer::Serialize(out, str);
    }

    template <typename T>
    static void Deserialize(std::istream& inputStream, T& value) {
        BinarySerializer::Deserialize(inputStream, value);
    }

    template <typename T>
    static void Deserialize(std::istream& inputStream, std::vector<T>& vec) {
        BinarySerializer::Deserialize(inputStream, vec);
    }

    static void Deserialize(std::istream& inputStream, std::string& str) {
        BinarySerializer::Deserialize(inputStream, str);
    }
};

}  // namespace Hardware
