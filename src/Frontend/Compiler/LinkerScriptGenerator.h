#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include "CompilerFrontend.h"

#ifndef TARGET_WASM

class LinkerScriptGenerator {
public:
    struct AppLinkerOptions {
        bool hasGpu = false;
        size_t numOverlays = 0;
    };

    static std::string StripComments(const std::string& source);
    static CompilerFrontend::HardwareRequirements DetectHardwareRequirements(const std::string& code);
    static size_t DetectOverlayCount(std::string_view code);
    static std::string GenerateLinkerScript(const std::string& code);
    static std::string GenerateAppLinkerScript(bool hasGpu = false, size_t numOverlays = 0);
    static std::string GenerateAppLinkerScript(const AppLinkerOptions& options);
};

#endif
