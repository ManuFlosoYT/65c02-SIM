#pragma once
#include <string>
#include "CompilerFrontend.h"

#ifndef TARGET_WASM

class LinkerScriptGenerator {
public:
    static std::string StripComments(const std::string& source);
    static CompilerFrontend::HardwareRequirements DetectHardwareRequirements(const std::string& code);
    static std::string GenerateLinkerScript(const std::string& code);
    static std::string GenerateAppLinkerScript();
};

#endif
