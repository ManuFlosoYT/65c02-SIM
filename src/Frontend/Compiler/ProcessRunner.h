#pragma once
#include <string>

#ifndef TARGET_WASM

class ProcessRunner {
public:
    static int RunCommand(const std::string& cmd, std::string& output);
    static int PlatformRunCommand(const std::string& cmd, const std::string& logFile);
};

#endif
