#pragma once

#include <string>

namespace Control {

class SymbolTable;

class DwarfNmParser {
public:
    static bool Parse(const std::string& debugLineOutput, const std::string& nmOutput, SymbolTable& table);
};

} // namespace Control
