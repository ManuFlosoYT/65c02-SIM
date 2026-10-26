#include "Hardware/Core/DwarfNmParser.h"
#include "Hardware/Core/SymbolTable.h"
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Control {

bool DwarfNmParser::Parse(const std::string& debugLineOutput, const std::string& nmOutput, SymbolTable& table) {
    std::istringstream stream(debugLineOutput);
    std::string line;

    std::unordered_map<unsigned int, std::string> incDirs;
    std::unordered_map<unsigned int, std::string> unitFiles;
    std::unordered_map<unsigned int, unsigned int> unitFileDir;
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
            unitFileDir.clear();
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
        } else if (line.find("dir_index:") != std::string::npos) {
            size_t pos = line.find("dir_index:");
            try {
                unitFileDir[curFileIdx] = static_cast<unsigned int>(std::stoul(line.substr(pos + 10), nullptr, 0));
            } catch (...) {
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
                        auto itDir = unitFileDir.find(fIdx);
                        if (itDir != unitFileDir.end()) {
                            auto itInc = incDirs.find(itDir->second);
                            if (itInc != incDirs.end() && !itInc->second.empty()) {
                                fPath = (std::filesystem::path(itInc->second) / fPath).lexically_normal().string();
                            }
                        }
                        unsigned int globalFid = 0;
                        auto itF = pathToFileId.find(fPath);
                        if (itF == pathToFileId.end()) {
                            globalFid = static_cast<unsigned int>(table.files.size() + 1);
                            pathToFileId[fPath] = globalFid;
                            DbgFile df;
                            df.id = globalFid;
                            df.name = fPath;
                            table.files[globalFid] = df;
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
        unsigned int lineId = static_cast<unsigned int>(table.lines.size() + 1);
        DbgLine dl;
        dl.id = lineId;
        dl.fileId = e.fileId;
        dl.line = e.line;
        table.lines[lineId] = dl;

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
            table.addrToLineId[static_cast<uint16_t>(a)] = lineId;
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
                        unsigned int symId = static_cast<unsigned int>(table.syms.size() + 1);
                        DbgSym sym;
                        sym.id = symId;
                        sym.name = symName;
                        sym.val = static_cast<int>(symAddr);
                        table.syms[symId] = sym;

                        uint16_t u16Addr = static_cast<uint16_t>(symAddr);
                        if (table.addrToSymId.find(u16Addr) == table.addrToSymId.end() || symType == 'T' || symType == 't') {
                            table.addrToSymId[u16Addr] = symId;
                        }
                        if (table.nameToSymId.find(symName) == table.nameToSymId.end() || symType == 'T' || symType == 't') {
                            table.nameToSymId[symName] = symId;
                        }
                    }
                } catch (...) {
                }
            }
        }
    }

    table.loaded = (!table.lines.empty() || !table.syms.empty());
    return table.loaded;
}

} // namespace Control
