#pragma once
#include <iostream>
#include <cstring>
#include <type_traits>
#include <array>

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
        static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types can be serialized using this helper");
        std::array<char, sizeof(T)> buffer{};
        std::memcpy(buffer.data(), &value, sizeof(T));
        out.write(buffer.data(), sizeof(T));
    }

    template <typename T>
    static void Deserialize(std::istream& inputStream, T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types can be deserialized using this helper");
        std::array<char, sizeof(T)> buffer{};
        inputStream.read(buffer.data(), sizeof(T));
        std::memcpy(&value, buffer.data(), sizeof(T));
    }
};

}  // namespace Hardware
