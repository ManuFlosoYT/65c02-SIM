#pragma once
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>
#include <string>

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
        out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template <typename T>
    static void Serialize(std::ostream& out, const std::vector<T>& vec) {
        static_assert(std::is_trivially_copyable_v<T>, "Only vectors of trivially copyable types can be serialized using this helper");
        out.write(reinterpret_cast<const char*>(vec.data()), static_cast<std::streamsize>(vec.size() * sizeof(T)));
    }

    template <typename T>
    static void Deserialize(std::istream& inputStream, T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types can be deserialized using this helper");
        inputStream.read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    template <typename T>
    static void Deserialize(std::istream& inputStream, std::vector<T>& vec) {
        static_assert(std::is_trivially_copyable_v<T>, "Only vectors of trivially copyable types can be deserialized using this helper");
        inputStream.read(reinterpret_cast<char*>(vec.data()), static_cast<std::streamsize>(vec.size() * sizeof(T)));
    }

    static void Serialize(std::ostream& out, const std::string& str) {
        auto len = static_cast<uint32_t>(str.length());
        Serialize(out, len);
        out.write(str.c_str(), static_cast<std::streamsize>(len));
    }

    static void Deserialize(std::istream& inputStream, std::string& str) {
        uint32_t len = 0;
        Deserialize(inputStream, len);
        str.assign(len, '\0');
        inputStream.read(str.data(), static_cast<std::streamsize>(len));
    }
};

}  // namespace Hardware
