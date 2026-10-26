#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#include "Frontend/Compiler/ToolchainLocator.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "embedded_llvm_mos.h"

namespace fs = std::filesystem;

std::string CompilerVFS::tempDir;
std::string CompilerVFS::compilerPath;

static void WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
    fs::path tempPath(path);
    fs::create_directories(tempPath.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out.write(std::bit_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
}

void CompilerVFS::Initialize() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    fs::path tempRoot = fs::temp_directory_path() / "65c02_sim_toolchain";
    fs::create_directories(tempRoot);
    tempDir = tempRoot.string();
    compilerPath = ToolchainLocator::FindCompiler();

    std::cout << "[CompilerVFS] Initializing at " << tempDir << " with compiler: " << compilerPath << "\n";

    for (const auto& [vpath, data] : embedded_llvm_mos_files) {
        WriteFile((tempRoot / vpath).string(), data);
    }
}

void CompilerVFS::EnsureInitialized() {
    Initialize();
}

void CompilerVFS::Cleanup() {
    if (!tempDir.empty()) {
        std::error_code errorCode;
        fs::remove_all(tempDir, errorCode);
        tempDir.clear();
        compilerPath.clear();
    }
}

std::string CompilerVFS::GetTempDir() {
    EnsureInitialized();
    return tempDir;
}

std::string CompilerVFS::GetCompilerExecutable() {
    EnsureInitialized();
    return compilerPath;
}

#endif
