#pragma once

#include <string>
#include <unordered_map>

namespace Control {

class SymbolTable;

class DbgFormatParser {
public:
    static bool ParseFile(const std::string& path, SymbolTable& table);
    static void ProcessRecord(SymbolTable& table, const std::string& type, std::unordered_map<std::string, std::string>& attrs);
    static std::unordered_map<std::string, std::string> ParseAttributes(const std::string& attrString);
};

} // namespace Control
