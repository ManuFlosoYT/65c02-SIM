#include "Hardware/Core/SourceCodeCache.h"
#include "Hardware/Core/SymbolTable.h"
#include <array>
#include <fstream>

namespace Control {

void SourceCodeCache::Clear() {
    fileLines.clear();
}

const std::string& SourceCodeCache::GetSourceLine(const SymbolTable& table, unsigned int fileId, unsigned int line) const {
    static const std::string emptyLine;
    if (line == 0) {
        return emptyLine;
    }
    auto it = fileLines.find(fileId);
    if (it == fileLines.end()) {
        const auto* file = table.GetFile(fileId);
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

} // namespace Control
