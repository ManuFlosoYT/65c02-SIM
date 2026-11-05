#ifndef TARGET_WASM
#include "Frontend/Compiler/ToolchainLocator.h"
#include "Frontend/Platform/PlatformUtils.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

fs::path ToolchainLocator::GetExecutableDir() {
    return Frontend::Platform::GetExecutableDirectory();
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
            fs::path p = base / Frontend::Platform::AppendExecutableExtension(binName);
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
    };

    for (const auto& root : searchRoots) {
        for (const auto& rel : relativeCandidates) {
            fs::path candidate = root / Frontend::Platform::AppendExecutableExtension(rel);
            if (fs::exists(candidate)) {
                return NormalizePath(candidate);
            }
        }
    }

    const char* pathEnv = std::getenv("PATH");
    if (pathEnv != nullptr) {
        std::stringstream stringStream(pathEnv);
        std::string dir;
        char delimiter = Frontend::Platform::GetEnvPathDelimiter();
        while (std::getline(stringStream, dir, delimiter)) {
            if (dir.empty()) {
                continue;
            }
            for (const auto* name : {"mos-common-clang", "mos-clang"}) {
                fs::path p = fs::path(dir) / Frontend::Platform::AppendExecutableExtension(name);
                if (fs::exists(p)) {
                    return NormalizePath(p);
                }
            }
        }
    }

    return Frontend::Platform::AppendExecutableExtension("mos-common-clang");
}

std::string ToolchainLocator::FindTool(const std::string& toolName) {
    std::string compiler = FindCompiler();
    fs::path binDir = fs::path(compiler).parent_path();

    std::string exeName = Frontend::Platform::AppendExecutableExtension(toolName);
    std::vector<std::string> names = (exeName != toolName) ? std::vector<std::string>{exeName, toolName} : std::vector<std::string>{toolName};

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
        char delimiter = Frontend::Platform::GetEnvPathDelimiter();
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

    return Frontend::Platform::AppendExecutableExtension(toolName);
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
