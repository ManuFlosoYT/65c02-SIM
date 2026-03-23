#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifndef EMSCRIPTEN

class CompilerFrontend {
public:
    enum class BuildType : std::uint8_t {
        C,
        Assembly
    };

    struct BuildResult {
        bool success = false;
        std::string log;
        std::vector<uint8_t> binary;
        std::string dbgFile;
    };

    static BuildResult Compile(BuildType type, const std::string& code, const std::string& sourceName = "test.c");
    
private:
    static std::string GenerateCFG(const std::string& code);
};

#endif
