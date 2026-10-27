#pragma once

#include "Token.h"
#include <cstdint>
#include <string>
#include <variant>
#include <stdexcept>
#include <utility>

namespace System::Hardware::Scripting::Lang {

struct Callable {
    std::string name;
};

using Value = std::variant<std::monostate, int64_t, std::string, bool, Callable>;

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(Token token, const std::string& message)
        : std::runtime_error(message), token(std::move(token)) {}
    Token token;
};

inline bool isTruthy(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return false;
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    if (std::holds_alternative<int64_t>(value)) {
        return std::get<int64_t>(value) != 0;
    }
    return true;
}

inline bool isEqual(const Value& a, const Value& b) {
    if (a.index() != b.index()) {
        return false;
    }
    if (std::holds_alternative<std::monostate>(a)) {
        return true;
    }
    if (std::holds_alternative<bool>(a)) {
        return std::get<bool>(a) == std::get<bool>(b);
    }
    if (std::holds_alternative<int64_t>(a)) {
        return std::get<int64_t>(a) == std::get<int64_t>(b);
    }
    if (std::holds_alternative<std::string>(a)) {
        return std::get<std::string>(a) == std::get<std::string>(b);
    }
    return false;
}

inline std::string stringify(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<Callable>(value)) {
        return "<fn " + std::get<Callable>(value).name + ">";
    }
    return "";
}

} // namespace System::Hardware::Scripting::Lang
