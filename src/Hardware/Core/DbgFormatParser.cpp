#include "Hardware/Core/DbgFormatParser.h"
#include "Hardware/Core/SymbolTable.h"
#include <cstdint>
#include <fstream>
#include <sstream>

namespace Control {

std::unordered_map<std::string, std::string> DbgFormatParser::ParseAttributes(const std::string& attrString) {
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

void DbgFormatParser::ProcessRecord(SymbolTable& table, const std::string& type, std::unordered_map<std::string, std::string>& attrs) {
    if (type == "file") {
        DbgFile fileObj;
        fileObj.id = ParseUIntDef(attrs["id"]);
        fileObj.name = attrs["name"];
        fileObj.size = ParseUIntDef(attrs["size"]);
        table.files[fileObj.id] = fileObj;
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
        table.syms[symObj.id] = symObj;
    } else if (type == "span") {
        DbgSpan spanObj;
        spanObj.id = ParseUIntDef(attrs["id"]);
        spanObj.segId = ParseUIntDef(attrs["seg"]);
        spanObj.start = ParseUIntDef(attrs["start"]);
        spanObj.size = ParseUIntDef(attrs["size"]);
        table.spans[spanObj.id] = spanObj;
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
        table.lines[lineObj.id] = lineObj;
    } else if (type == "seg") {
        DbgSeg segObj;
        segObj.id = ParseUIntDef(attrs["id"]);
        segObj.name = attrs["name"];
        segObj.start = ParseUIntDef(attrs["start"]);
        segObj.size = ParseUIntDef(attrs["size"]);
        segObj.addrsize = attrs["addrsize"];
        segObj.type = attrs["type"];
        table.segs[segObj.id] = segObj;
    }
}

bool DbgFormatParser::ParseFile(const std::string& path, SymbolTable& table) {
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
        
        attrsStr.erase(0, attrsStr.find_first_not_of(" \t"));
        
        auto attrs = ParseAttributes(attrsStr);
        ProcessRecord(table, type, attrs);
    }

    for (const auto& [symId, sym] : table.syms) {
        if (sym.val >= 0 && sym.name.size() > 0 && sym.name[0] != '@') {
            table.addrToSymId[static_cast<uint16_t>(sym.val)] = symId;
        }
        if (!sym.name.empty()) {
            table.nameToSymId[sym.name] = symId;
        }
    }

    for (const auto& [lineId, dbgLine] : table.lines) {
        for (unsigned int spanId : dbgLine.spanIds) {
            auto itSpan = table.spans.find(spanId);
            if (itSpan != table.spans.end()) {
                auto itSeg = table.segs.find(itSpan->second.segId);
                if (itSeg != table.segs.end()) {
                    auto absAddr = static_cast<uint16_t>(itSeg->second.start + itSpan->second.start);
                    table.addrToLineId[absAddr] = lineId;
                }
            }
        }
    }

    table.loaded = true;
    return true;
}

} // namespace Control
