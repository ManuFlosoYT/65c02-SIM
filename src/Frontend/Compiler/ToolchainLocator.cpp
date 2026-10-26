#ifndef TARGET_WASM
#include "Frontend/Compiler/ToolchainLocator.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

fs::path ToolchainLocator::GetExecutableDir() {
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

std::string ToolchainLocator::NormalizePath(const fs::path& p) {
    return fs::absolute(p).lexically_normal().string();
}

std::string ToolchainLocator::FindCompiler() {
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

std::string ToolchainLocator::FindTool(const std::string& toolName) {
    std::string compiler = FindCompiler();
    fs::path binDir = fs::path(compiler).parent_path();

    std::vector<std::string> names = { toolName };
#ifdef _WIN32
    if (!toolName.ends_with(".exe")) {
        names.insert(names.begin(), toolName + ".exe");
    }
#endif

    for (const auto& name : names) {
        fs::path p = binDir / name;
        if (fs::exists(p)) {
            return NormalizePath(p);
        }
    }

    const char* pathEnv = std::getenv("PATH");
    if (pathEnv != nullptr) {
        std::stringstream ss(pathEnv);
        std::string dir;
#ifdef _WIN32
        char delimiter = ';';
#else
        char delimiter = ':';
#endif
        while (std::getline(ss, dir, delimiter)) {
            if (dir.empty()) continue;
            for (const auto& name : names) {
                fs::path p = fs::path(dir) / name;
                if (fs::exists(p)) {
                    return NormalizePath(p);
                }
            }
        }
    }

#ifdef _WIN32
    if (!toolName.ends_with(".exe")) {
        return toolName + ".exe";
    }
#endif
    return toolName;
}

std::string ToolchainLocator::FindObjcopy() {
    return FindTool("llvm-objcopy");
}

std::string ToolchainLocator::FindDwarfdump() {
    return FindTool("llvm-dwarfdump");
}

std::string ToolchainLocator::FindNm() {
    return FindTool("llvm-nm");
}

#endif
