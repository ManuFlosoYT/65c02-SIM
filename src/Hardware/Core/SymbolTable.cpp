#include "Hardware/Core/SymbolTable.h"
#include "Hardware/Core/DbgFormatParser.h"
#include "Hardware/Core/DwarfNmParser.h"

namespace Control {

SymbolTable::ElfLoader SymbolTable::s_elfLoader = nullptr;

void SymbolTable::SetElfLoader(ElfLoader loader) {
    s_elfLoader = std::move(loader);
}

void SymbolTable::Clear() {
    files.clear();
    syms.clear();
    spans.clear();
    lines.clear();
    segs.clear();
    sourceCache.Clear();
    addrToSymId.clear();
    addrToLineId.clear();
    nameToSymId.clear();
    loaded = false;
}

bool SymbolTable::LoadFromFile(const std::string& path) {
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".elf") {
        return LoadFromELF(path);
    }
    Clear();
    return DbgFormatParser::ParseFile(path, *this);
}

const DbgSym* SymbolTable::GetSymbolAtAddress(uint16_t address) const {
    auto iter = addrToSymId.find(address);
    if (iter != addrToSymId.end()) {
        auto symIt = syms.find(iter->second);
        if (symIt != syms.end()) {
            return &symIt->second;
        }
    }
    return nullptr;
}

const DbgLine* SymbolTable::GetLineForAddress(uint16_t address) const {
    auto iter = addrToLineId.find(address);
    if (iter != addrToLineId.end()) {
        auto valIt = lines.find(iter->second);
        if (valIt != lines.end()) {
            return &valIt->second;
        }
    }
    return nullptr;
}

const DbgFile* SymbolTable::GetFile(unsigned int fileId) const {
    auto iter = files.find(fileId);
    return iter != files.end() ? &iter->second : nullptr;
}

const DbgSym* SymbolTable::FindSymbol(const std::string& name) const {
    if (name.empty()) {
        return nullptr;
    }

    auto lookup = [this](const std::string& key) -> const DbgSym* {
        auto it = nameToSymId.find(key);
        if (it != nameToSymId.end()) {
            auto symIt = syms.find(it->second);
            if (symIt != syms.end()) {
                return &symIt->second;
            }
        }
        return nullptr;
    };

    if (const auto* sym = lookup(name)) {
        return sym;
    }
    if (const auto* sym = lookup("_" + name)) {
        return sym;
    }
    if (name.front() == '_') {
        if (const auto* sym = lookup(name.substr(1))) {
            return sym;
        }
    }

    return nullptr;
}

bool SymbolTable::LoadFromELF(const std::string& elfPath) {
    if (s_elfLoader) {
        return s_elfLoader(*this, elfPath);
    }
    return false;
}

bool SymbolTable::LoadFromDebugInfo(const std::string& debugLineOutput, const std::string& nmOutput) {
    Clear();
    return DwarfNmParser::Parse(debugLineOutput, nmOutput, *this);
}

const std::string& SymbolTable::GetSourceLine(unsigned int fileId, unsigned int line) const {
    return sourceCache.GetSourceLine(*this, fileId, line);
}

}  // namespace Control
