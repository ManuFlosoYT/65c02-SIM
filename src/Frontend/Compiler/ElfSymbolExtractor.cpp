#ifndef TARGET_WASM
#include "ElfSymbolExtractor.h"
#include "CompilerFrontend.h"
#include "CompilerVFS.h"
#include "ProcessRunner.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

bool ElfSymbolExtractor::LoadELF(Control::SymbolTable& table, const std::string& elfPath) {
    Control::SymbolTable::SetElfLoader(&CompilerFrontend::LoadELF);

    std::string compiler = CompilerVFS::GetCompilerExecutable();
    std::filesystem::path binDir = std::filesystem::path(compiler).parent_path();

    std::string dwarfdump = (binDir / "llvm-dwarfdump").string();
    std::string nm = (binDir / "llvm-nm").string();
#ifdef _WIN32
    dwarfdump += ".exe";
    nm += ".exe";
#endif
    if (!std::filesystem::exists(dwarfdump)) {
        dwarfdump = "llvm-dwarfdump";
    }
    if (!std::filesystem::exists(nm)) {
        nm = "llvm-nm";
    }

    std::string lineDump;
    std::string nmDump;
    ProcessRunner::RunCommand(dwarfdump + " --debug-line \"" + elfPath + "\"", lineDump);
    ProcessRunner::RunCommand(nm + " -n \"" + elfPath + "\"", nmDump);

    return table.LoadFromDebugInfo(lineDump, nmDump);
}

bool ElfSymbolExtractor::GenerateDbgFromMap(const std::string& mapPath, const std::string& dbgPath) {
    std::ifstream in(mapPath);
    if (!in) {
        return false;
    }
    std::ofstream out(dbgPath);
    if (!out) {
        return false;
    }

    std::unordered_set<std::string> seen;
    std::string line;
    unsigned int id = 0;
    while (std::getline(in, line)) {
        std::stringstream stringStream(line);
        std::string vmaStr;
        std::string lmaStr;
        std::string sizeStr;
        std::string alignStr;
        std::string symName;
        if (stringStream >> vmaStr >> lmaStr >> sizeStr >> alignStr >> symName) {
            if (symName.empty() || symName.find('=') != std::string::npos ||
                symName.find('.') != std::string::npos || symName.find(':') != std::string::npos ||
                symName.find('<') != std::string::npos || symName.find('/') != std::string::npos ||
                symName.find('>') != std::string::npos || symName.find('(') != std::string::npos ||
                symName.find(')') != std::string::npos) {
                continue;
            }
            if (!seen.insert(symName).second) {
                continue;
            }
            try {
                size_t idx = 0;
                unsigned long addr = std::stoul(vmaStr, &idx, 16);
                if (idx == vmaStr.size()) {
                    out << "sym\tid=" << id++ << ",name=\"" << symName << "\",val=0x"
                        << std::hex << std::uppercase << addr << "\n";
                }
            } catch (...) {
            }
        }
    }
    return id > 0;
}

#endif
