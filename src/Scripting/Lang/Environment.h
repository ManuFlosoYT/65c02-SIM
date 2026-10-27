#pragma once

#include "Value.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace System::Hardware::Scripting::Lang {

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
