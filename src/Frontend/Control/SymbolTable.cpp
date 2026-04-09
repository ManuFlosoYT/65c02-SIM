#include "Frontend/Control/SymbolTable.h"

#include <fstream>

namespace Control {

void SymbolTable::Clear() {
    files.clear();
    mods.clear();
    types.clear();
    syms.clear();
    spans.clear();
    lines.clear();
    scopes.clear();
    segs.clear();
    addrToSymId.clear();
    addrToLineId.clear();
    loaded = false;
}

static std::unordered_map<std::string, std::string> ParseAttributes(const std::string& attrString) {
    std::unordered_map<std::string, std::string> attrs;
    size_t pos = 0;
    while (pos < attrString.length()) {
        size_t eqPos = attrString.find('=', pos);
        if (eqPos == std::string::npos) {
            break;
        }
        std::string key = attrString.substr(pos, eqPos - pos);
        pos = eqPos + 1;
        std::string val;
        if (pos < attrString.length() && attrString[pos] == '"') {
            pos++;
            size_t endQuote = attrString.find('"', pos);
            if (endQuote != std::string::npos) {
                val = attrString.substr(pos, endQuote - pos);
                pos = endQuote + 1;
                if (pos < attrString.length() && attrString[pos] == ',') {
                    pos++;
                }
            } else {
                val = attrString.substr(pos);
                pos = attrString.length();
            }
        } else {
            size_t commaPos = attrString.find(',', pos);
            if (commaPos != std::string::npos) {
                val = attrString.substr(pos, commaPos - pos);
                pos = commaPos + 1;
            } else {
                val = attrString.substr(pos);
                pos = attrString.length();
            }
        }
        attrs[key] = val;
    }
    return attrs;
}

static int ParseIntDef(const std::string& strVal, int def = -1) {
    if (strVal.empty()) {
        return def;
    }
    try {
        if (strVal.size() > 2 && strVal[0] == '0' && (strVal[1] == 'x' || strVal[1] == 'X')) {
            return std::stoi(strVal.substr(2), nullptr, 16);
        }
        return std::stoi(strVal);
    } catch (...) {
        return def;
    }
}

static unsigned int ParseUIntDef(const std::string& strVal, unsigned int def = 0) {
    if (strVal.empty()) {
        return def;
    }
    try {
        if (strVal.size() > 2 && strVal[0] == '0' && (strVal[1] == 'x' || strVal[1] == 'X')) {
            return std::stoul(strVal.substr(2), nullptr, 16);
        }
        return std::stoul(strVal);
    } catch (...) {
        return def;
    }
}

void SymbolTable::ProcessRecord(const std::string& type, std::unordered_map<std::string, std::string>& attrs) {
    if (type == "file") {
        DbgFile fileObj;
        fileObj.id = ParseUIntDef(attrs["id"]);
        fileObj.name = attrs["name"];
        fileObj.size = ParseUIntDef(attrs["size"]);
        files[fileObj.id] = fileObj;
    } else if (type == "mod") {
        DbgMod modObj;
        modObj.id = ParseUIntDef(attrs["id"]);
        modObj.name = attrs["name"];
        modObj.fileId = ParseUIntDef(attrs["file"]);
        mods[modObj.id] = modObj;
    } else if (type == "type") {
        DbgType typeObj;
        typeObj.id = ParseUIntDef(attrs["id"]);
        typeObj.val = attrs["val"];
        types[typeObj.id] = typeObj;
    } else if (type == "sym") {
        DbgSym symObj;
        symObj.id = ParseUIntDef(attrs["id"]);
        symObj.name = attrs["name"];
        symObj.addrsize = attrs["addrsize"];
        symObj.typeId = ParseIntDef(attrs["type"]);
        symObj.scopeId = ParseIntDef(attrs["scope"]);
        symObj.defLineId = ParseIntDef(attrs["def"]);
        symObj.val = ParseIntDef(attrs["val"]);
        symObj.segId = ParseIntDef(attrs["seg"]);
        syms[symObj.id] = symObj;
    } else if (type == "span") {
        DbgSpan spanObj;
        spanObj.id = ParseUIntDef(attrs["id"]);
        spanObj.segId = ParseUIntDef(attrs["seg"]);
        spanObj.start = ParseUIntDef(attrs["start"]);
        spanObj.size = ParseUIntDef(attrs["size"]);
        spans[spanObj.id] = spanObj;
    } else if (type == "line") {
        DbgLine lineObj;
        lineObj.id = ParseUIntDef(attrs["id"]);
        lineObj.fileId = ParseUIntDef(attrs["file"]);
        lineObj.line = ParseUIntDef(attrs["line"]);
        
        std::string spanStr = attrs["span"];
        if (!spanStr.empty()) {
            size_t pos = 0;
            while (pos < spanStr.length()) {
                size_t plus = spanStr.find('+', pos);
                if (plus == std::string::npos) {
                    plus = spanStr.length();
                }
                std::string sId = spanStr.substr(pos, plus - pos);
                lineObj.spanIds.push_back(ParseUIntDef(sId));
                pos = plus + 1;
            }
        }
        lines[lineObj.id] = lineObj;
    } else if (type == "scope") {
        DbgScope scopeObj;
        scopeObj.id = ParseUIntDef(attrs["id"]);
        scopeObj.name = attrs["name"];
        scopeObj.type = attrs["type"];
        scopeObj.size = ParseUIntDef(attrs["size"]);
        scopeObj.parentId = ParseIntDef(attrs["parent"]);
        scopes[scopeObj.id] = scopeObj;
    } else if (type == "seg") {
        DbgSeg segObj;
        segObj.id = ParseUIntDef(attrs["id"]);
        segObj.name = attrs["name"];
        segObj.start = ParseUIntDef(attrs["start"]);
        segObj.size = ParseUIntDef(attrs["size"]);
        segObj.addrsize = attrs["addrsize"];
        segObj.type = attrs["type"];
        segs[segObj.id] = segObj;
    }
}

bool SymbolTable::LoadFromFile(const std::string& path) {
    Clear();
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        size_t splitPos = line.find('\t');
        if (splitPos == std::string::npos) {
            splitPos = line.find(' ');
        }
        if (splitPos == std::string::npos) {
            continue;
        }

        std::string type = line.substr(0, splitPos);
        std::string attrsStr = line.substr(splitPos + 1);
        
        // trim leading spaces from attrsStr
        attrsStr.erase(0, attrsStr.find_first_not_of(" \t"));
        
        auto attrs = ParseAttributes(attrsStr);
        ProcessRecord(type, attrs);
    }

    // Secondary pass to build lookup tables
    for (const auto& [symId, sym] : syms) {
        if (sym.val >= 0 && sym.name.size() > 0 && sym.name[0] != '@') {
            addrToSymId[static_cast<uint16_t>(sym.val)] = symId;
        }
    }

    // Line lookup build
    for (const auto& [lineId, dbgLine] : lines) {
        for (unsigned int spanId : dbgLine.spanIds) {
            auto itSpan = spans.find(spanId);
            if (itSpan != spans.end()) {
                auto itSeg = segs.find(itSpan->second.segId);
                if (itSeg != segs.end()) {
                    auto absAddr = static_cast<uint16_t>(itSeg->second.start + itSpan->second.start);
                    addrToLineId[absAddr] = lineId;
                }
            }
        }
    }

    loaded = true;
    return true;
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

const DbgMod* SymbolTable::GetMod(unsigned int modId) const {
    auto iter = mods.find(modId);
    return iter != mods.end() ? &iter->second : nullptr;
}

const DbgType* SymbolTable::GetType(unsigned int typeId) const {
    auto iter = types.find(typeId);
    return iter != types.end() ? &iter->second : nullptr;
}

const DbgScope* SymbolTable::GetScope(unsigned int scopeId) const {
    auto iter = scopes.find(scopeId);
    return iter != scopes.end() ? &iter->second : nullptr;
}

const DbgSeg* SymbolTable::GetSeg(unsigned int segId) const {
    auto iter = segs.find(segId);
    return iter != segs.end() ? &iter->second : nullptr;
}

}  // namespace Control
