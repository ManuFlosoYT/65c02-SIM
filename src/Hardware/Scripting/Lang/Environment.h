#pragma once

#include "Token.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <memory>
#include <stdexcept>

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

class Environment {
public:
    Environment();
    explicit Environment(std::shared_ptr<Environment> enclosing);

    void define(const std::string& name, Value value);
    Value get(const Token& name);
    void assign(const Token& name, const Value& value);

private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, Value> values;
};

} // namespace System::Hardware::Scripting::Lang
