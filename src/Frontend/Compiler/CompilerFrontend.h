#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Hardware/Core/SymbolTable.h"

#ifndef TARGET_WASM

class CompilerFrontend {
public:
    enum class BuildType : std::uint8_t {
        C,
        Assembly,
        MicroDOS
    };

    struct HardwareRequirements {
        bool gpu = false;
        bool sid = false;
        bool net = false;
        bool sd = false;
    };

    struct Diagnostic {
        std::string file;
        int line = 0;
        int column = 0;
        std::string message;
        bool isError = true;
    };

    struct BuildResult {
        bool success = false;
        std::string log;
        std::vector<uint8_t> binary;
        std::string symbolFile;
        std::string elfPath;
        Control::SymbolTable symbols;
        HardwareRequirements hardware;
        std::vector<Diagnostic> diagnostics;
    };

    static std::vector<Diagnostic> ParseDiagnostics(const std::string& log);

    static BuildResult Compile(BuildType type, const std::string& code, const std::string& sourceName = "test.c");
    static int RunCommand(const std::string& cmd, std::string& output);
    static bool LoadELF(Control::SymbolTable& table, const std::string& elfPath);
    
private:
    static std::string GenerateLinkerScript(const std::string& code);
    static std::string GenerateAppLinkerScript();
};

#endif
