#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifndef TARGET_WASM

class CompilerFrontend {
public:
    enum class BuildType : std::uint8_t {
        C,
        Assembly,
        MicroDOS
    };

    struct BuildResult {
        bool success = false;
        std::string log;
        std::vector<uint8_t> binary;
        std::string symbolFile;
    };

    static BuildResult Compile(BuildType type, const std::string& code, const std::string& sourceName = "test.c");
    
private:
    static std::string GenerateLinkerScript(const std::string& code);
    static std::string GenerateAppLinkerScript();
};

#endif
