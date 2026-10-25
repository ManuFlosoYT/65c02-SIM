#pragma once
#include <string>

#ifndef TARGET_WASM

class CompilerVFS {
public:
    static void Initialize();
    static void EnsureInitialized();
    static void Cleanup();
    static std::string GetTempDir();
    static std::string GetCompilerExecutable();

private:
    static std::string tempDir;
    static std::string compilerPath;
};

#endif

