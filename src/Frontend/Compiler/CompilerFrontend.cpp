#ifndef TARGET_WASM
#include "CompilerFrontend.h"
#include "CompilerVFS.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static std::string ReadFile(const std::string& path) {
    std::ifstream fileIn(path, std::ios::binary);
    if (!fileIn) {
        return "";
    }
    std::ostringstream contents;
    contents << fileIn.rdbuf();
    return contents.str();
}

static std::vector<uint8_t> ReadBinaryFile(const std::string& path) {
    std::ifstream fileIn(path, std::ios::binary | std::ios::ate);
    if (!fileIn) {
        return {};
    }
    const auto fileSize = fileIn.tellg();
    fileIn.seekg(0, std::ios::beg);
    std::vector<char> buffer(static_cast<size_t>(fileSize));
    fileIn.read(buffer.data(), fileSize);
    return {buffer.begin(), buffer.end()};
}

static void WriteTextFile(const std::string& path, const std::string& txt) {
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out << txt;
    }
}

#ifdef _WIN32
static int PlatformRunCommand(const std::string& cmd, const std::string& logFile) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    HANDLE hLog = CreateFileA(logFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &saAttr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hLog == INVALID_HANDLE_VALUE) {
        return -1;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hLog;
    si.hStdOutput = hLog;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    std::string mutableCmd = cmd;
    if (!CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hLog);
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hLog);

    return static_cast<int>(exitCode);
}
#else
static int PlatformRunCommand(const std::string& cmd, const std::string& logFile) {
    std::vector<std::string> args;
    std::stringstream stringStream(cmd);
    std::string item;
    while (stringStream >> item) {
        args.push_back(item);
    }

    if (args.empty()) {
        return -1;
    }

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        return -1;
    }
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

    std::vector<std::vector<char>> argBuffers;
    argBuffers.reserve(args.size());
    for (const auto& arg : args) {
        std::vector<char> buf(arg.begin(), arg.end());
        buf.push_back('\0');
        argBuffers.push_back(std::move(buf));
    }

    std::vector<char*> argv;
    argv.reserve(argBuffers.size() + 1);
    for (auto& buf : argBuffers) {
        argv.push_back(buf.data());
    }
    argv.push_back(nullptr);

    pid_t pid = 0;
    if (posix_spawn(&pid, argv.at(0), &actions, nullptr, argv.data(), nullptr) != 0) {
        posix_spawn_file_actions_destroy(&actions);
        return -1;
    }

    int status = 0;
    waitpid(pid, &status, 0);
    posix_spawn_file_actions_destroy(&actions);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

static int RunSysCommand(const std::string& cmd, std::string& log) {
    std::string tempLog = CompilerVFS::GetTempDir() + "/temp.log";
    int result = PlatformRunCommand(cmd, tempLog);
    log += ReadFile(tempLog);
    return result;
}

static bool IsSizeOverflowError(const std::string& log) {
    return log.find("will not fit in region") != std::string::npos ||
           log.find("overflowed by") != std::string::npos ||
           log.find("address range overlaps with") != std::string::npos;
}

std::string CompilerFrontend::GenerateLinkerScript(const std::string& code) {
    bool hasGpu = code.find("--gpu") != std::string::npos || code.find("Libs/GPU.h") != std::string::npos;
    bool hasDoubleBuffer = code.find("--double-buffer") != std::string::npos;
    bool hasNet = code.find("--net") != std::string::npos || code.find("Libs/NET.h") != std::string::npos;
    bool hasSid = code.find("--sid") != std::string::npos || code.find("Libs/SID.h") != std::string::npos;

    struct Block {
        uint32_t start;
        uint32_t end;
        std::string name;
    };
    std::vector<Block> res = {
        {0x5000, 0x5003, "ACIA"},
        {0x6000, 0x600F, "VIA"}
    };

    if (hasSid) {
        res.push_back({0x4800, 0x481F, "SID"});
    }
    if (hasNet) {
        res.push_back({0x5004, 0x5007, "ESP8266"});
    }
    if (hasGpu || hasDoubleBuffer) {
        res.push_back({0x2000, 0x3FFF, "GPU VRAM Front Buffer"});
    }
    if (hasDoubleBuffer) {
        res.push_back({0x6010, 0x790F, "GPU VRAM Back Buffer"});
    }

    std::ranges::sort(res, [](const Block& first, const Block& second) {
        return first.start < second.start;
    });

    std::vector<Block> merged;
    for (const auto& block : res) {
        if (merged.empty()) {
            merged.push_back(block);
        } else {
            auto& prev = merged.back();
            if (block.start <= prev.end + 1) {
                prev.end = std::max(prev.end, block.end);
                prev.name += " + " + block.name;
            } else {
                merged.push_back(block);
            }
        }
    }

    std::vector<Block> ram;
    uint32_t current = 0x0400;
    uint32_t ramEnd = 0x8000;
    for (const auto& block : merged) {
        if (current < block.start) {
            ram.push_back({current, block.start - 1, ""});
        }
        current = std::max(current, block.end + 1);
    }
    if (current < ramEnd) {
        ram.push_back({current, ramEnd - 1, ""});
    }

    std::ostringstream ld;
    ld << "/* Auto-generated link.ld for 65c02-SIM with LLVM-MOS */\n\n";
    ld << "__rc0 = 0x00;\n";
    ld << "INCLUDE imag-regs.ld\n";
    ld << "ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")\n";
    ld << "ASSERT(__rc31 == 0x1f, \"Error en mapeo de Zero Page.\")\n\n";

    ld << "MEMORY {\n";
    ld << "    zp           (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x100 - (__rc31 + 1)\n";
    ld << "    input_buffer (rw) : ORIGIN = 0x0300,     LENGTH = 0x0100\n";

    for (size_t i = 0; i < ram.size(); i++) {
        uint32_t size = ram.at(i).end - ram.at(i).start + 1;
        std::string name = "ram_" + std::to_string(i + 1);
        ld << "    " << name << " (rw) : ORIGIN = 0x" << std::hex << std::uppercase << ram.at(i).start
           << ", LENGTH = 0x" << size << "\n";
    }

    ld << "    rom          (rx) : ORIGIN = 0x8000,     LENGTH = 0x8000\n";
    ld << "}\n\n";

    std::string mainRam = "ram_1";
    ld << "REGION_ALIAS(\"c_readonly\", rom)\n";
    ld << "REGION_ALIAS(\"c_writeable\", " << mainRam << ")\n\n";

    ld << "SECTIONS {\n";
    ld << "    .input_buffer 0x0300 (NOLOAD) : {\n";
    ld << "        KEEP(*(SORT_BY_NAME(.input_buffer*)))\n";
    ld << "        *(.input_buffer)\n";
    ld << "    } > input_buffer\n\n";
    ld << "    INCLUDE c.ld\n\n";
    ld << "    .vectors 0xFFFA : {\n";
    ld << "        SHORT(DEFINED(_nmi) ? _nmi : 0x0F00)\n";
    ld << "        SHORT(DEFINED(_start) ? _start : (DEFINED(_reset) ? _reset : 0x8000))\n";
    ld << "        SHORT(DEFINED(IRQ_HANDLER) ? IRQ_HANDLER : (DEFINED(_irq) ? _irq : 0))\n";
    ld << "    } > rom\n";
    ld << "}\n\n";

    uint32_t stackStart = ram.back().end;
    ld << "__stack = 0x" << std::hex << std::uppercase << stackStart << ";\n\n";
    ld << "OUTPUT_FORMAT {\n";
    ld << "    FULL(rom)\n";
    ld << "}\n";

    return ld.str();
}

CompilerFrontend::BuildResult CompilerFrontend::Compile(BuildType type, const std::string& code,
                                                        const std::string& sourceName) {
    BuildResult res;
    if (code.empty()) {
        res.log = "Error: Empty source code.\n";
        return res;
    }

    std::string tempDir = CompilerVFS::GetTempDir();
    if (tempDir.empty()) {
        res.log = "Error: Compiler VFS not initialized.";
        return res;
    }

    std::string baseName = sourceName;
    size_t lastDot = baseName.find_last_of('.');
    if (lastDot != std::string::npos) {
        baseName = baseName.substr(0, lastDot);
    }
    size_t lastSlash = baseName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseName = baseName.substr(lastSlash + 1);
    }
    if (baseName.empty()) {
        baseName = "out";
    }

    std::string srcFile = tempDir + "/" + baseName + (type == BuildType::C ? ".c" : ".s");
    WriteTextFile(srcFile, code);

    std::string linkScript = tempDir + "/link.ld";
    WriteTextFile(linkScript, GenerateLinkerScript(code));

    std::string binFile = tempDir + "/out.bin";
    std::string compiler = CompilerVFS::GetCompilerExecutable();

    const std::array<const char*, 2> optLevels = {"-Os", "-Oz"};
    bool buildOk = false;

    for (const auto* opt : optLevels) {
        std::error_code ec;
        fs::remove(binFile, ec);

        std::string cmd = compiler + " -mcpu=mosw65c02 " + opt + " -T " + linkScript + " -I " + tempDir + "/include";
        if (type == BuildType::Assembly) {
            cmd += " -nostartfiles " + srcFile + " -o " + binFile;
        } else {
            cmd +=
                " -flto -ffunction-sections -fdata-sections -fno-unroll-loops -fnonreentrant -Wl,--gc-sections "
                "-Wl,--lto-O2 -mlto-zp=200 ";
            cmd += srcFile;
            cmd += " ";
            cmd += tempDir;
            cmd += "/Linker/bios.s ";
            cmd += tempDir;
            cmd += "/include/Libs/BIOS.c";
            if (code.find("SD.h") != std::string::npos) {
                cmd += " " + tempDir + "/include/Libs/SD.c " + tempDir + "/include/Libs/fatfs/ff.c " + tempDir +
                       "/include/Libs/fatfs/diskio.c -D ENABLE_SD_SUPPORT=1";
            }
            cmd += " -o " + binFile;
        }

        res.log += "> " + cmd + "\n";
        std::string stepLog;
        int exitCode = RunSysCommand(cmd, stepLog);
        res.log += stepLog;

        if (exitCode == 0 && fs::exists(binFile) && fs::file_size(binFile) > 0) {
            buildOk = true;
            res.log += "Optimization " + std::string(opt) + " succeeded.\n";
            break;
        }

        if (IsSizeOverflowError(stepLog)) {
            res.log += "Optimization " + std::string(opt) + " exceeded memory. Falling back...\n";
        } else {
            res.log += "Compilation error at " + std::string(opt) + " (not a size issue). Aborting without fallback.\n";
            return res;
        }
    }

    if (!buildOk) {
        res.log += "Error: Compilation failed for all optimization levels (-Os, -Oz).\n";
        return res;
    }

    res.binary = ReadBinaryFile(binFile);
    if (res.binary.empty()) {
        res.log += "Error: Output binary is empty.\n";
        return res;
    }

    res.success = true;
    res.log += "Compilation Successful.\n";
    return res;
}
#endif
