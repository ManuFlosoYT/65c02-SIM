#include "Frontend/Control/SymbolTable.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>

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
    fileLines.clear();
    addrToSymId.clear();
    addrToLineId.clear();
    nameToSymId.clear();
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
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".elf") {
        return LoadFromELF(path);
    }
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
        if (!sym.name.empty()) {
            nameToSymId[sym.name] = symId;
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

    std::istringstream stream(debugLineOutput);
    std::string line;

    std::unordered_map<unsigned int, std::string> incDirs;
    std::unordered_map<unsigned int, std::string> unitFiles;
    std::unordered_map<std::string, unsigned int> pathToFileId;

    bool inTable = false;
    unsigned int curFileIdx = 0;

    struct LineEntry {
        uint16_t addr = 0;
        unsigned int line = 0;
        unsigned int fileId = 0;
        bool isEndSeq = false;
    };
    std::vector<LineEntry> entries;

    while (std::getline(stream, line)) {
        if (line.rfind("debug_line[", 0) == 0) {
            incDirs.clear();
            unitFiles.clear();
            inTable = false;
        } else if (line.find("include_directories[") != std::string::npos) {
            size_t b1 = line.find('[');
            size_t b2 = line.find(']');
            size_t q1 = line.find('"', b2);
            size_t q2 = line.rfind('"');
            if (b1 != std::string::npos && b2 != std::string::npos && q1 != std::string::npos && q2 > q1) {
                unsigned int dirIdx = static_cast<unsigned int>(std::stoul(line.substr(b1 + 1, b2 - b1 - 1)));
                incDirs[dirIdx] = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (line.find("file_names[") != std::string::npos) {
            size_t b1 = line.find('[');
            size_t b2 = line.find(']');
            if (b1 != std::string::npos && b2 != std::string::npos) {
                curFileIdx = static_cast<unsigned int>(std::stoul(line.substr(b1 + 1, b2 - b1 - 1)));
            }
        } else if (line.find("name:") != std::string::npos && line.find("file_names") == std::string::npos) {
            size_t q1 = line.find('"');
            size_t q2 = line.rfind('"');
            if (q1 != std::string::npos && q2 > q1) {
                unitFiles[curFileIdx] = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (line.find("Address ") != std::string::npos && line.find("Line ") != std::string::npos) {
            inTable = true;
        } else if (inTable) {
            if (line.empty() || line[0] == '\r') {
                inTable = false;
                continue;
            }
            std::istringstream ls(line);
            std::string addrStr;
            unsigned int lineNum = 0;
            unsigned int colNum = 0;
            unsigned int fIdx = 0;
            if (ls >> addrStr >> lineNum >> colNum >> fIdx) {
                if (addrStr.size() >= 3 && addrStr[0] == '0' && (addrStr[1] == 'x' || addrStr[1] == 'X')) {
                    try {
                        uint64_t fullAddr = std::stoull(addrStr, nullptr, 16);
                        std::string rest;
                        std::getline(ls, rest);
                        bool isEnd = (rest.find("end_sequence") != std::string::npos);

                        std::string fPath = unitFiles[fIdx];
                        unsigned int globalFid = 0;
                        auto itF = pathToFileId.find(fPath);
                        if (itF == pathToFileId.end()) {
                            globalFid = static_cast<unsigned int>(files.size() + 1);
                            pathToFileId[fPath] = globalFid;
                            DbgFile df;
                            df.id = globalFid;
                            df.name = fPath;
                            files[globalFid] = df;
                        } else {
                            globalFid = itF->second;
                        }

                        entries.push_back({
                            .addr = static_cast<uint16_t>(fullAddr),
                            .line = lineNum,
                            .fileId = globalFid,
                            .isEndSeq = isEnd
                        });
                    } catch (...) {
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        if (e.line == 0) {
            continue;
        }
        unsigned int lineId = static_cast<unsigned int>(lines.size() + 1);
        DbgLine dl;
        dl.id = lineId;
        dl.fileId = e.fileId;
        dl.line = e.line;
        lines[lineId] = dl;

        uint16_t endAddr = e.addr;
        if (!e.isEndSeq && i + 1 < entries.size()) {
            const auto& nextE = entries[i + 1];
            if (nextE.addr > e.addr && (nextE.addr - e.addr) < 1000) {
                endAddr = nextE.addr;
            }
        }
        if (endAddr <= e.addr) {
            endAddr = e.addr + 1;
        }
        for (uint32_t a = e.addr; a < endAddr && a <= 0xFFFF; ++a) {
            addrToLineId[static_cast<uint16_t>(a)] = lineId;
        }
    }

    std::istringstream nmStream(nmOutput);
    std::string nline;
    while (std::getline(nmStream, nline)) {
        std::istringstream nls(nline);
        std::string nAddrStr;
        char symType = 0;
        std::string symName;
        if (nls >> nAddrStr >> symType >> symName) {
            if (!symName.empty() && symName[0] != '.' && symName[0] != '$') {
                try {
                    uint64_t symAddr = std::stoull(nAddrStr, nullptr, 16);
                    if (symAddr <= 0xFFFF) {
                        unsigned int symId = static_cast<unsigned int>(syms.size() + 1);
                        DbgSym sym;
                        sym.id = symId;
                        sym.name = symName;
                        sym.val = static_cast<int>(symAddr);
                        syms[symId] = sym;

                        uint16_t u16Addr = static_cast<uint16_t>(symAddr);
                        if (addrToSymId.find(u16Addr) == addrToSymId.end() || symType == 'T' || symType == 't') {
                            addrToSymId[u16Addr] = symId;
                        }
                        if (nameToSymId.find(symName) == nameToSymId.end() || symType == 'T' || symType == 't') {
                            nameToSymId[symName] = symId;
                        }
                    }
                } catch (...) {
                }
            }
        }
    }

    loaded = (!lines.empty() || !syms.empty());
    return loaded;
}

const std::string& SymbolTable::GetSourceLine(unsigned int fileId, unsigned int line) const {
    static const std::string emptyLine;
    if (line == 0) {
        return emptyLine;
    }
    auto it = fileLines.find(fileId);
    if (it == fileLines.end()) {
        const auto* file = GetFile(fileId);
        if (!file) {
            return emptyLine;
        }
        std::vector<std::string> linesVec;
        std::string path = file->name;
        std::ifstream fileIn(path);
        if (!fileIn.is_open()) {
            const std::array<const char*, 3> prefixes = {"", "sdk/", "sdk/microdos/"};
            for (const auto* prefix : prefixes) {
                std::string testPath = std::string(prefix) + path;
                fileIn.open(testPath);
                if (fileIn.is_open()) {
                    break;
                }
            }
        }
        if (fileIn.is_open()) {
            std::string s;
            while (std::getline(fileIn, s)) {
                linesVec.push_back(s);
            }
        }
        it = fileLines.emplace(fileId, std::move(linesVec)).first;
    }
    if (it != fileLines.end() && line <= it->second.size()) {
        return it->second.at(line - 1);
    }
    return emptyLine;
}

}  // namespace Control
