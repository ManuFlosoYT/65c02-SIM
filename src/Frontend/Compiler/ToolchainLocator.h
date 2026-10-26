#pragma once

#ifndef TARGET_WASM

#include <filesystem>
#include <string>

class ToolchainLocator {
public:
    static std::string FindCompiler();
    static std::string FindTool(const std::string& toolName);
    static std::string FindObjcopy();
    static std::string FindDwarfdump();
    static std::string FindNm();

    static std::filesystem::path GetExecutableDir();
    static std::string NormalizePath(const std::filesystem::path& p);
};

#endif
