#ifndef EMSCRIPTEN
#include "CompilerFrontend.h"
#include "CC65VFS.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <array>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>
#endif

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

    HANDLE hLog = CreateFileA(logFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &saAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    // Basic argument splitter (respects spaces, no quotes needed for our current paths)
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
    std::string tempLog = CC65VFS::GetTempDir() + "/temp.log";
    int result = PlatformRunCommand(cmd, tempLog);
    log += ReadFile(tempLog);
    return result;
}

std::string CompilerFrontend::GenerateCFG(const std::string& code) {
    // Ported from Linker/generate_cfg.py
    bool hasGpu = code.find("--gpu") != std::string::npos || code.find("Libs/GPU.h") != std::string::npos;
    bool hasDoubleBuffer = code.find("--double-buffer") != std::string::npos;
    bool hasNet = code.find("--net") != std::string::npos || code.find("Libs/NET.h") != std::string::npos;

    struct Block { uint32_t start, end; std::string name; };
    std::vector<Block> res = {
        {0x4800, 0x481F, "SID"},
        {0x5000, 0x5003, "ACIA"},
        {0x5004, 0x5007, "ESP8266"},
        {0x6000, 0x600F, "VIA"}
    };

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
    uint32_t current = 0x0400; // ram_start
    uint32_t ram_end = 0x8000;
    for (const auto& block : merged) {
        if (current < block.start) {
            ram.push_back({current, block.start - 1, ""});
        }
        current = std::max(current, block.end + 1);
    }
    if (current < ram_end) {
        ram.push_back({current, ram_end - 1, ""});
    }

    std::ostringstream cfg;
    cfg << "MEMORY {\n";
    cfg << "    ZP:           start = $0000, size = $0100, type = rw, define = yes, file = \"\";\n";
    cfg << "    INPUT_BUFFER: start = $0300, size = $0100, type = rw, file = \"\";\n";
    
    for (size_t i = 0; i < ram.size(); i++) {
        uint32_t size = ram.at(i).end - ram.at(i).start + 1;
        std::string name = "RAM_" + std::to_string(i + 1);
        cfg << "    " << name << ": start = $" << std::hex << ram.at(i).start << ", size = $" << size << ", type = rw, define = yes, file = \"\";\n";
    }

    cfg << "    ROM:          start = $8000, size = $7FFA, type = ro, define = yes, file = %O, fill = yes, fillval = $FF;\n";
    cfg << "    RESETVEC:     start = $FFFA, size = $0006, type = ro, define = yes, file = %O, fill = yes, fillval = $FF;\n";
    cfg << "}\n\n";

    cfg << "SEGMENTS {\n";
    cfg << "    ZEROPAGE:     load = ZP,  type = zp;\n";
    cfg << "    INPUT_BUFFER: load = INPUT_BUFFER, type = rw;\n";
    cfg << "    HEADER:       load = ROM, type = ro;\n";
    
    std::string main_ram = "RAM_1";
    cfg << "    DATA:         load = ROM, run = " << main_ram << ", type = rw, define = yes;\n";
    cfg << "    BSS:          load = " << main_ram << ", type = bss, define = yes;\n";
    cfg << "    CODE:         load = ROM, type = ro;\n";
    cfg << "    RODATA:       load = ROM, type = ro;\n";
    cfg << "    BIOS:         load = ROM, type = ro;\n";
    cfg << "    STARTUP:      load = ROM, type = ro, optional = yes;\n";
    cfg << "    ONCE:         load = ROM, type = ro, optional = yes;\n";
    cfg << "    RESETVEC:     load = RESETVEC, type = ro, optional = yes;\n";
    cfg << "}\n\n";

    cfg << "FEATURES {\n";
    cfg << "    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;\n";
    cfg << "    CONDES: type = destructor,  label = __DESTRUCTOR_TABLE__,  count = __DESTRUCTOR_COUNT__,  segment = RODATA;\n";
    cfg << "}\n\n";

    cfg << "SYMBOLS {\n";
    uint32_t stack_start = ram.back().end;
    cfg << "    __STACKSTART__: type = weak, value = $" << std::hex << stack_start << ";\n";
    cfg << "}\n";

    return cfg.str();
}

static int CompileCC65(const std::string& cc65, const std::string& tempDir, const std::string& srcFile, std::string& log) {
    std::string cmd = cc65 + " -I " + tempDir + "/include -O --cpu 65C02 -g " + srcFile + " -o " + tempDir + "/test.s";
    log += "> " + cmd + "\n";
    return RunSysCommand(cmd, log);
}

static int AssembleFile(const std::string& ca65, const std::string& src, const std::string& obj, std::string& log) {
    std::string cmd = ca65 + " -g -t none " + src + " -o " + obj;
    log += "> " + cmd + "\n";
    return RunSysCommand(cmd, log);
}

static int CompileExtraLibs(const std::string& cc65, const std::string& ca65, const std::string& tempDir, const std::string& code, std::string& extraObjs, std::string& log) {
    if (code.find("SD.h") != std::string::npos) {
        const std::array<std::string, 3> files = { "Libs/SD.c", "Libs/fatfs/ff.c", "Libs/fatfs/diskio.c" };
        const std::array<std::string, 3> objNames = { "sd", "ff", "diskio" };
        for (size_t i = 0; i < files.size(); ++i) {
            std::string s_cmd = cc65;
            s_cmd += " -I ";
            s_cmd += tempDir;
            s_cmd += "/include -O --cpu 65C02 ";
            s_cmd += tempDir;
            s_cmd += "/include/";
            s_cmd += files.at(i);
            s_cmd += " -o ";
            s_cmd += tempDir;
            s_cmd += "/";
            s_cmd += objNames.at(i);
            s_cmd += ".s";
            
            log += "> " + s_cmd + "\n";
            if (RunSysCommand(s_cmd, log) != 0) {
                return -1;
            }

            std::string o_cmd = ca65;
            o_cmd += " -g -t none ";
            o_cmd += tempDir;
            o_cmd += "/";
            o_cmd += objNames.at(i);
            o_cmd += ".s -o ";
            o_cmd += tempDir;
            o_cmd += "/";
            o_cmd += objNames.at(i);
            o_cmd += ".o";
            
            log += "> " + o_cmd + "\n";
            if (RunSysCommand(o_cmd, log) != 0) {
                return -1;
            }

            extraObjs += tempDir + "/" + objNames.at(i) + ".o ";
        }
    }
    return 0;
}

CompilerFrontend::BuildResult CompilerFrontend::Compile(BuildType type, const std::string& code) {
    BuildResult res;
    std::string tempDir = CC65VFS::GetTempDir();
    if (tempDir.empty()) {
        res.log = "Error: CC65 VFS not initialized.";
        return res;
    }

    std::string srcFile = tempDir + (type == BuildType::C ? "/test.c" : "/test.s");
    WriteTextFile(srcFile, code);

    std::string ext;
#ifdef _WIN32
    ext = ".exe";
#endif

    std::string cc65 = tempDir + "/cc65" + ext;
    std::string ca65 = tempDir + "/ca65" + ext;
    std::string ld65 = tempDir + "/ld65" + ext;
    
    if (type == BuildType::C) {
        if (CompileCC65(cc65, tempDir, srcFile, res.log) != 0) {
            return res;
        }
    }

    if (AssembleFile(ca65, tempDir + "/test.s", tempDir + "/test.o", res.log) != 0) {
        return res;
    }

    if (AssembleFile(ca65, tempDir + "/Linker/bios.s", tempDir + "/bios.o", res.log) != 0) {
        return res;
    }

    if (type == BuildType::C) {
        if (AssembleFile(ca65, tempDir + "/Linker/C-Runtime.s", tempDir + "/cr.o", res.log) != 0) {
            return res;
        }
    }

    std::string extraObjs;
    if (type == BuildType::C) {
        if (CompileExtraLibs(cc65, ca65, tempDir, code, extraObjs, res.log) != 0) {
            return res;
        }
    }

    std::string cfgFile = tempDir + "/dynamic.cfg";
    WriteTextFile(cfgFile, GenerateCFG(code));

    // Link
    {
        std::string objs = tempDir + "/bios.o ";
        if (type == BuildType::C) {
            objs += tempDir + "/cr.o ";
        }
        objs += tempDir + "/test.o " + extraObjs;
        
        std::string cmd = ld65 + " -C " + cfgFile + " -o " + tempDir + "/out.bin " + objs + (type == BuildType::C ? " " + tempDir + "/lib/none.lib" : "");
        res.log += "> " + cmd + "\n";
        if (RunSysCommand(cmd, res.log) != 0) {
            return res;
        }
    }

    res.binary = ReadBinaryFile(tempDir + "/out.bin");
    if (res.binary.empty()) {
        res.log += "Error: Output binary is empty.\n";
        return res;
    }

    res.success = true;
    res.log += "Compilation Successful.\n";
    return res;
}
#endif
