#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerVFS.h"
#include <array>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

static fs::path GetExecutableDir() {
#ifdef _WIN32
    std::array<char, MAX_PATH> buffer{};
    if (GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) > 0) {
        return fs::path(buffer.data()).parent_path();
    }
#else
    std::error_code ec;
    auto exePath = fs::read_symlink("/proc/self/exe", ec);
    if (!ec && !exePath.empty()) {
        return exePath.parent_path();
    }
#endif
    return fs::current_path();
}

static std::string NormalizePath(const fs::path& p) {
    return fs::absolute(p).lexically_normal().string();
}

static std::string FindCompiler() {
    const char* envPath = std::getenv("MOS_CLANG");
    if (envPath != nullptr && fs::exists(envPath)) {
        return NormalizePath(envPath);
    }

    const char* llvmMosDir = std::getenv("LLVM_MOS_DIR");
    if (llvmMosDir == nullptr) {
        llvmMosDir = std::getenv("LLVM_MOS_ROOT");
    }
    if (llvmMosDir != nullptr) {
        fs::path base(llvmMosDir);
        for (const auto* binName : {"bin/mos-common-clang", "bin/mos-clang", "mos-common-clang", "mos-clang"}) {
            fs::path p = base / binName;
            if (fs::exists(p)) {
                return NormalizePath(p);
            }
        }
    }

#ifdef MOS_CLANG_DEFAULT_PATH
    if (fs::exists(MOS_CLANG_DEFAULT_PATH)) {
        return NormalizePath(MOS_CLANG_DEFAULT_PATH);
    }
#endif

    const auto exeDir = GetExecutableDir();
    const std::vector<fs::path> searchRoots = {
        exeDir,
        exeDir.parent_path(),
        exeDir.parent_path().parent_path(),
        fs::current_path()
    };

    const std::vector<std::string> relativeCandidates = {
        "build/llvm_mos/llvm-mos/bin/mos-common-clang",
        "build/llvm_mos/llvm-mos/bin/mos-clang",
        "llvm_mos/llvm-mos/bin/mos-common-clang",
        "llvm_mos/llvm-mos/bin/mos-clang",
        "output/toolchain/bin/mos-common-clang",
        "output/toolchain/bin/mos-clang",
        "toolchain/bin/mos-common-clang",
        "toolchain/bin/mos-clang",
        "bin/mos-common-clang",
        "bin/mos-clang",
#ifdef _WIN32
        "build/llvm_mos/llvm-mos/bin/mos-common-clang.exe",
        "build/llvm_mos/llvm-mos/bin/mos-clang.exe",
        "llvm_mos/llvm-mos/bin/mos-common-clang.exe",
        "llvm_mos/llvm-mos/bin/mos-clang.exe",
        "output/toolchain/bin/mos-common-clang.exe",
        "output/toolchain/bin/mos-clang.exe",
        "toolchain/bin/mos-common-clang.exe",
        "toolchain/bin/mos-clang.exe",
        "bin/mos-common-clang.exe",
        "bin/mos-clang.exe",
#endif
    };

    for (const auto& root : searchRoots) {
        for (const auto& rel : relativeCandidates) {
            fs::path candidate = root / rel;
            if (fs::exists(candidate)) {
                return NormalizePath(candidate);
            }
        }
    }

    const char* pathEnv = std::getenv("PATH");
    if (pathEnv != nullptr) {
        std::stringstream stringStream(pathEnv);
        std::string dir;
#ifdef _WIN32
        char delimiter = ';';
#else
        char delimiter = ':';
#endif
        while (std::getline(stringStream, dir, delimiter)) {
            if (dir.empty()) {
                continue;
            }
            for (const auto* name : {"mos-common-clang", "mos-clang"
#ifdef _WIN32
                                     , "mos-common-clang.exe", "mos-clang.exe"
#endif
            }) {
                fs::path p = fs::path(dir) / name;
                if (fs::exists(p)) {
                    return NormalizePath(p);
                }
            }
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

