#pragma once
#include <string>
#include <vector>
#include "CompilerFrontend.h"

#ifndef TARGET_WASM

class CompilerDiagnosticParser {
public:
    static std::vector<CompilerFrontend::Diagnostic> ParseDiagnostics(const std::string& log);
    static bool IsSizeOverflowError(const std::string& log);
};

#endif
