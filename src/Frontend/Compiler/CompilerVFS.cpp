#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "embedded_llvm_mos.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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

static std::string FindCompiler() {
    const char* envPath = std::getenv("MOS_CLANG");
    if (envPath != nullptr && fs::exists(envPath)) {
        return envPath;
    }

    const std::vector<std::string> candidates = {
        "build/llvm_mos/llvm-mos/bin/mos-common-clang",
        "build/llvm_mos/llvm-mos/bin/mos-clang",
        "output/toolchain/bin/mos-common-clang",
        "output/toolchain/bin/mos-clang",
#ifdef _WIN32
        "build/llvm_mos/llvm-mos/bin/mos-common-clang.exe",
        "build/llvm_mos/llvm-mos/bin/mos-clang.exe",
        "output/toolchain/bin/mos-common-clang.exe",
        "output/toolchain/bin/mos-clang.exe",
#endif
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::canonical(candidate).string();
        }
    }

#ifdef _WIN32
    return "mos-common-clang.exe";
#else
    return "mos-common-clang";
#endif
}

void CompilerVFS::Initialize() {
    fs::path tempRoot = fs::temp_directory_path() / "65c02_sim_toolchain";
    fs::create_directories(tempRoot);
    tempDir = tempRoot.string();
    compilerPath = FindCompiler();

    std::cout << "[CompilerVFS] Initializing at " << tempDir << " with compiler: " << compilerPath << "\n";

    for (const auto& [vpath, data] : embedded_llvm_mos_files) {
        WriteFile((tempRoot / vpath).string(), data);
    }
}

void CompilerVFS::Cleanup() {
    if (!tempDir.empty()) {
        std::error_code errorCode;
        fs::remove_all(tempDir, errorCode);
    }
}

std::string CompilerVFS::GetTempDir() {
    return tempDir;
}

std::string CompilerVFS::GetCompilerExecutable() {
    return compilerPath;
}

#endif

