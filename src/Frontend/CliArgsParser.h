#pragma once

#include <span>
#include <string>

namespace Frontend {

struct Args {
    bool headless = false;
    bool profileSID = false;
    bool sidSound = false;
    bool sidDevice = true;
    std::string scriptPath;
    std::string romPath;
    std::string dumpMemPath;
    int runCycles = 0;
};

struct ParseArgsResult {
    Args args;
    bool ok = true;
    bool showUsage = false;
    std::string error;
};

class CliArgsParser {
public:
    static ParseArgsResult Parse(std::span<const char* const> argv);
    static void PrintUsage(const char* exeName);
};

}  // namespace Frontend
