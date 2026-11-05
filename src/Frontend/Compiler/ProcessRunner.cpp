#ifndef TARGET_WASM
#include "ProcessRunner.h"
#include "CompilerVFS.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

static std::string ReadFile(const std::string& path) {
    std::ifstream fileIn(path, std::ios::binary);
    if (!fileIn) {
        return "";
    }
    std::ostringstream contents;
    contents << fileIn.rdbuf();
    return contents.str();
}

int ProcessRunner::RunCommand(const std::string& cmd, std::string& output) {
    std::string tempLog = CompilerVFS::GetTempDir() + "/temp.log";
    int result = PlatformRunCommand(cmd, tempLog);
    output = ReadFile(tempLog);
    return result;
}

#endif
