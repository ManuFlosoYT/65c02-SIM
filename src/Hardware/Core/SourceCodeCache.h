#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Control {

class SymbolTable;

class SourceCodeCache {
public:
    SourceCodeCache() = default;
    ~SourceCodeCache() = default;
    SourceCodeCache(const SourceCodeCache&) = default;
    SourceCodeCache& operator=(const SourceCodeCache&) = default;
    SourceCodeCache(SourceCodeCache&&) noexcept = default;
    SourceCodeCache& operator=(SourceCodeCache&&) noexcept = default;

    void Clear();
    const std::string& GetSourceLine(const SymbolTable& table, unsigned int fileId, unsigned int line) const;

private:
    mutable std::unordered_map<unsigned int, std::vector<std::string>> fileLines;
};

} // namespace Control
