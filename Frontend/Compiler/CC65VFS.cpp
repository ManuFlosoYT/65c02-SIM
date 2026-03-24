#ifndef TARGET_WASM
#include "CC65VFS.h"
#include "embedded_cc65.h"
#include "cc65_exe.h"
#include "ca65_exe.h"
#include "ld65_exe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

static std::string g_TempDir;

static void WriteExecutable(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
        
#ifndef _WIN32
        chmod(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
    }
}

static void WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
    fs::path p(path);
    fs::create_directories(p.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

void CC65VFS::Initialize() {
    fs::path tempRoot = fs::temp_directory_path() / "65c02_sim_cc65";
    fs::create_directories(tempRoot);
    g_TempDir = tempRoot.string();

    std::cout << "[CC65VFS] Initializing at " << g_TempDir << std::endl;

    // Write internal compilers
#ifdef _WIN32
    WriteExecutable((tempRoot / "cc65.exe").string(), cc65_exe_data);
    WriteExecutable((tempRoot / "ca65.exe").string(), ca65_exe_data);
    WriteExecutable((tempRoot / "ld65.exe").string(), ld65_exe_data);
#else
    WriteExecutable((tempRoot / "cc65").string(), cc65_exe_data);
    WriteExecutable((tempRoot / "ca65").string(), ca65_exe_data);
    WriteExecutable((tempRoot / "ld65").string(), ld65_exe_data);
#endif

    // Write all VFS files (none.lib, headers, etc)
    for (const auto& [vpath, data] : embedded_cc65_files) {
        WriteFile((tempRoot / vpath).string(), data);
    }

    // Set Environment Variables so cc65 finds its headers and libs
    std::string incPath = (tempRoot / "include").string();
    std::string asmIncPath = (tempRoot / "asminc").string();
    std::string libPath = (tempRoot / "lib").string();
    
#ifdef _WIN32
    _putenv_s("CC65_INC", incPath.c_str());
    _putenv_s("CA65_INC", asmIncPath.c_str());
    _putenv_s("LD65_LIB", libPath.c_str());
    _putenv_s("LD65_OBJ", libPath.c_str());
#else
    setenv("CC65_INC", incPath.c_str(), 1);
    setenv("CA65_INC", asmIncPath.c_str(), 1);
    setenv("LD65_LIB", libPath.c_str(), 1);
    setenv("LD65_OBJ", libPath.c_str(), 1);
#endif
}

void CC65VFS::Cleanup() {
    if (!g_TempDir.empty()) {
        std::error_code ec;
        fs::remove_all(g_TempDir, ec);
    }
}

std::string CC65VFS::GetTempDir() {
    return g_TempDir;
}

#endif
