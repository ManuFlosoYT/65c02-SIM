#ifndef TARGET_WASM
#include "CompilerFrontend.h"
#include "CompilerVFS.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

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
        WriteTextFile(logFile, "Error: Failed to execute process: error code " + std::to_string(GetLastError()) + "\n");
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
    int spawnResult = posix_spawnp(&pid, argv.at(0), &actions, nullptr, argv.data(), nullptr);
    if (spawnResult != 0) {
        posix_spawn_file_actions_destroy(&actions);
        WriteTextFile(logFile, "Error: Failed to execute compiler '" + args.at(0) + "': " + std::strerror(spawnResult) + "\n");
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

int CompilerFrontend::RunCommand(const std::string& cmd, std::string& output) {
    std::string tempLog = CompilerVFS::GetTempDir() + "/temp.log";
    int result = PlatformRunCommand(cmd, tempLog);
    output = ReadFile(tempLog);
    return result;
}

bool CompilerFrontend::LoadELF(Control::SymbolTable& table, const std::string& elfPath) {
    Control::SymbolTable::SetElfLoader(&CompilerFrontend::LoadELF);

    std::string compiler = CompilerVFS::GetCompilerExecutable();
    fs::path binDir = fs::path(compiler).parent_path();

    std::string dwarfdump = (binDir / "llvm-dwarfdump").string();
    std::string nm = (binDir / "llvm-nm").string();
#ifdef _WIN32
    dwarfdump += ".exe";
    nm += ".exe";
#endif
    if (!fs::exists(dwarfdump)) {
        dwarfdump = "llvm-dwarfdump";
    }
    if (!fs::exists(nm)) {
        nm = "llvm-nm";
    }

    std::string lineDump;
    std::string nmDump;
    RunCommand(dwarfdump + " --debug-line \"" + elfPath + "\"", lineDump);
    RunCommand(nm + " -n \"" + elfPath + "\"", nmDump);

    return table.LoadFromDebugInfo(lineDump, nmDump);
}

static bool IsSizeOverflowError(const std::string& log) {
    return log.find("will not fit in region") != std::string::npos ||
           log.find("overflowed by") != std::string::npos ||
           log.find("address range overlaps with") != std::string::npos;
}

static bool GenerateDbgFromMap(const std::string& mapPath, const std::string& dbgPath) {
    std::ifstream in(mapPath);
    if (!in) {
        return false;
    }
    std::ofstream out(dbgPath);
    if (!out) {
        return false;
    }

    std::unordered_set<std::string> seen;
    std::string line;
    unsigned int id = 0;
    while (std::getline(in, line)) {
        std::stringstream stringStream(line);
        std::string vmaStr;
        std::string lmaStr;
        std::string sizeStr;
        std::string alignStr;
        std::string symName;
        if (stringStream >> vmaStr >> lmaStr >> sizeStr >> alignStr >> symName) {
            if (symName.empty() || symName.find('=') != std::string::npos ||
                symName.find('.') != std::string::npos || symName.find(':') != std::string::npos ||
                symName.find('<') != std::string::npos || symName.find('/') != std::string::npos ||
                symName.find('>') != std::string::npos || symName.find('(') != std::string::npos ||
                symName.find(')') != std::string::npos) {
                continue;
            }
            if (!seen.insert(symName).second) {
                continue;
            }
            try {
                size_t idx = 0;
                unsigned long addr = std::stoul(vmaStr, &idx, 16);
                if (idx == vmaStr.size()) {
                    out << "sym\tid=" << id++ << ",name=\"" << symName << "\",val=0x"
                        << std::hex << std::uppercase << addr << "\n";
                }
            } catch (...) {
            }
        }
    }
    return id > 0;
}

std::string CompilerFrontend::GenerateLinkerScript(const std::string& code) {
    bool hasGpu = code.find("--gpu") != std::string::npos || code.find("GPU.h") != std::string::npos;
    bool hasDoubleBuffer =
        code.find("--double-buffer") != std::string::npos || code.find("GPUDoubleBuffer.h") != std::string::npos;
    bool hasNet = code.find("--net") != std::string::npos || code.find("NET.h") != std::string::npos;
    bool hasSid = code.find("--sid") != std::string::npos || code.find("SID.h") != std::string::npos;

    struct Block {
        uint32_t start;
        uint32_t end;
        std::string name;
    };
    std::vector<Block> res = {
        {.start = 0x5000, .end = 0x5003, .name = "ACIA"},
        {.start = 0x5008, .end = 0x500B, .name = "SD Card"},
        {.start = 0x6000, .end = 0x600F, .name = "VIA"}
    };

    if (hasSid) {
        res.push_back({.start = 0x4800, .end = 0x481F, .name = "SID"});
    }
    if (hasNet) {
        res.push_back({.start = 0x5004, .end = 0x5007, .name = "ESP8266"});
    }
    if (hasGpu || hasDoubleBuffer) {
        res.push_back({.start = 0x2000, .end = 0x3FFF, .name = "GPU VRAM Front Buffer"});
    }
    if (hasDoubleBuffer) {
        res.push_back({.start = 0x6010, .end = 0x790F, .name = "GPU VRAM Back Buffer"});
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
            ram.push_back({.start = current, .end = block.start - 1, .name = ""});
        }
        current = std::max(current, block.end + 1);
    }
    if (current < ramEnd) {
        ram.push_back({.start = current, .end = ramEnd - 1, .name = ""});
    }

    std::ostringstream ld;
    ld << "/* Auto-generated link.ld for 65c02-SIM with LLVM-MOS */\n\n";
    ld << "__rc0 = 0x00;\n";
    ld << "INCLUDE imag-regs.ld\n";
    ld << "ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")\n";
    ld << "ASSERT(__rc31 == 0x1f, \"Error en mapeo de Zero Page.\")\n\n";

    ld << "MEMORY {\n";
    ld << "    zp           (rw) : ORIGIN = __rc31 + 1, LENGTH = 0xFC - (__rc31 + 1)\n";
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

    return ld.str();
}

std::string CompilerFrontend::GenerateAppLinkerScript() {
    std::ostringstream ld;
    ld << "/* Auto-generated link.ld for microDOS apps with LLVM-MOS */\n\n";
    ld << "__rc0 = 0x00;\n";
    ld << "INCLUDE imag-regs.ld\n";
    ld << "ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")\n";
    ld << "ASSERT(__rc31 <= 0x5F, \"Registros virtuales exceden Zero Page de app.\")\n\n";
    ld << "MEMORY {\n";
    ld << "    zp   (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x60 - (__rc31 + 1)\n";
    ld << "    core (rx) : ORIGIN = 0x0200,   LENGTH = 0x4600\n";
    ld << "    bss  (rw) : ORIGIN = 0x6010,  LENGTH = 0x1BF0\n";
    ld << "}\n\n";
    ld << "REGION_ALIAS(\"c_readonly\", core)\n";
    ld << "REGION_ALIAS(\"c_writeable\", bss)\n";
    ld << "EXTERN(__do_init_stack)\n";
    ld << "EXTERN(__do_copy_data)\n";
    ld << "EXTERN(__do_copy_zp_data)\n";
    ld << "EXTERN(__do_zero_bss)\n";
    ld << "EXTERN(__do_zero_zp_bss)\n\n";
    ld << "SECTIONS {\n";
    ld << "    .header 0x0200 : {\n";
    ld << "        KEEP(*(SORT_BY_NAME(.header*)))\n";
    ld << "        KEEP(*(.header))\n";
    ld << "    } > core\n\n";
    ld << "    INCLUDE c.ld\n";
    ld << "    PROVIDE(__CORE_SIZE__ = LOADADDR(.data) + SIZEOF(.data) - 0x0200);\n";
    ld << "    PROVIDE(_CORE_SIZE__ = __CORE_SIZE__);\n";
    ld << "}\n\n";
    ld << "__stack = 0x7BFF;\n\n";
    ld << "/* OS ABI Jump Table Symbols */\n";

    struct SymbolDef {
        const char* name;
        uint16_t offset;
    };
    static constexpr std::array<SymbolDef, 33> symbols = {{
        {.name = "print_str", .offset = 0x00},
        {.name = "println", .offset = 0x03},
        {.name = "print_num", .offset = 0x06},
        {.name = "print_hex_byte", .offset = 0x09},
        {.name = "bios_getchar", .offset = 0x0C},
        {.name = "bios_putchar", .offset = 0x0F},
        {.name = "sd_open", .offset = 0x12},
        {.name = "sd_read", .offset = 0x15},
        {.name = "sd_write", .offset = 0x18},
        {.name = "sd_close", .offset = 0x1B},
        {.name = "sd_puts", .offset = 0x1E},
        {.name = "sd_getc", .offset = 0x21},
        {.name = "sd_getcwd", .offset = 0x24},
        {.name = "sd_chdir", .offset = 0x27},
        {.name = "net_send", .offset = 0x2A},
        {.name = "net_cmd", .offset = 0x2D},
        {.name = "net_send_num", .offset = 0x30},
        {.name = "os_load_app_page", .offset = 0x33},
        {.name = "sid_write", .offset = 0x36},
        {.name = "sid_reset", .offset = 0x39},
        {.name = "sd_opendir", .offset = 0x3C},
        {.name = "sd_readdir", .offset = 0x3F},
        {.name = "sd_closedir", .offset = 0x42},
        {.name = "sd_mkdir", .offset = 0x45},
        {.name = "sd_remove", .offset = 0x48},
        {.name = "sd_exists", .offset = 0x4B},
        {.name = "net_wifi", .offset = 0x4E},
        {.name = "os_swap_save", .offset = 0x51},
        {.name = "os_swap_load", .offset = 0x54},
        {.name = "os_alloc", .offset = 0x57},
        {.name = "os_free", .offset = 0x5A},
        {.name = "bios_getchar_nb", .offset = 0x5D},
        {.name = "sd_seek", .offset = 0x60}
    }};

    constexpr uint16_t jumptable = 0xFF90;
    for (const auto& sym : symbols) {
        uint16_t addr = jumptable + sym.offset;
        ld << "PROVIDE(" << sym.name << " = 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << addr << ");\n";
        ld << "PROVIDE(_" << sym.name << " = 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << addr << ");\n";
    }

    ld << "\n";
    return ld.str();
}

std::vector<CompilerFrontend::Diagnostic> CompilerFrontend::ParseDiagnostics(const std::string& log) {
    std::vector<Diagnostic> diagnostics;
    std::istringstream stream(log);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        size_t markerPos = line.find(": error: ");
        size_t markerLen = 9;
        bool isError = true;

        if (markerPos == std::string::npos) {
            markerPos = line.find(": fatal error: ");
            markerLen = 15;
            isError = true;
        }
        if (markerPos == std::string::npos) {
            markerPos = line.find(": warning: ");
            markerLen = 11;
            isError = false;
        }
        if (markerPos == std::string::npos) {
            continue;
        }

        std::string message = line.substr(markerPos + markerLen);
        std::string prefix = line.substr(0, markerPos);

        size_t secondColon = prefix.rfind(':');
        if (secondColon == std::string::npos) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
            continue;
        }

        std::string secondPart = prefix.substr(secondColon + 1);
        bool secondIsDigit = !secondPart.empty() && std::ranges::all_of(secondPart, [](char c) {
            return std::isdigit(static_cast<unsigned char>(c)) != 0;
        });

        if (!secondIsDigit) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
            continue;
        }

        size_t firstColon = prefix.rfind(':', secondColon - 1);
        if (firstColon != std::string::npos && firstColon > 1) {
            std::string firstPart = prefix.substr(firstColon + 1, secondColon - firstColon - 1);
            bool firstIsDigit = !firstPart.empty() && std::ranges::all_of(firstPart, [](char c) {
                return std::isdigit(static_cast<unsigned char>(c)) != 0;
            });

            if (firstIsDigit) {
                try {
                    int lineNum = std::stoi(firstPart);
                    int colNum = std::stoi(secondPart);
                    std::string file = prefix.substr(0, firstColon);
                    diagnostics.push_back(
                        {.file = file, .line = lineNum, .column = colNum, .message = message, .isError = isError});
                    continue;
                } catch (const std::exception&) {
                }
            }
        }

        try {
            int lineNum = std::stoi(secondPart);
            std::string file = prefix.substr(0, secondColon);
            diagnostics.push_back(
                {.file = file, .line = lineNum, .column = 0, .message = message, .isError = isError});
        } catch (const std::exception&) {
            diagnostics.push_back({.file = prefix, .line = 0, .column = 0, .message = message, .isError = isError});
        }
    }

    return diagnostics;
}

CompilerFrontend::BuildResult CompilerFrontend::Compile(BuildType type, const std::string& code,
                                                        const std::string& sourceName) {
    BuildResult res;
    if (code.empty()) {
        res.log = "Error: Empty source code.\n";
        res.diagnostics = ParseDiagnostics(res.log);
        return res;
    }

    res.hardware.gpu = code.find("--gpu") != std::string::npos || code.find("GPU.h") != std::string::npos ||
                       code.find("--double-buffer") != std::string::npos ||
                       code.find("GPUDoubleBuffer.h") != std::string::npos;
    res.hardware.sid = code.find("--sid") != std::string::npos || code.find("SID.h") != std::string::npos;
    res.hardware.net = code.find("--net") != std::string::npos || code.find("NET.h") != std::string::npos;
    res.hardware.sd = code.find("--sd") != std::string::npos || code.find("SD.h") != std::string::npos;

    std::string tempDir = CompilerVFS::GetTempDir();
    if (tempDir.empty()) {
        res.log = "Error: Compiler VFS not initialized.";
        res.diagnostics = ParseDiagnostics(res.log);
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

    std::string srcFile = tempDir + "/" + baseName + (type == BuildType::Assembly ? ".s" : ".c");
    WriteTextFile(srcFile, code);

    std::string linkScript = tempDir + "/link.ld";
    if (type == BuildType::MicroDOS) {
        WriteTextFile(linkScript, GenerateAppLinkerScript());
    } else {
        WriteTextFile(linkScript, GenerateLinkerScript(code));
    }

    std::string binFile = tempDir + "/out.bin";
    std::string elfFile = tempDir + "/out.elf";
    std::string mapFile = tempDir + "/out.map";
    std::string dbgFile = tempDir + "/out.dbg";
    std::string compiler = CompilerVFS::GetCompilerExecutable();

    const std::array<const char*, 2> optLevels = {"-Os", "-Oz"};
    bool buildOk = false;

    for (const auto* opt : optLevels) {
        std::error_code ec;
        fs::remove(binFile, ec);
        fs::remove(elfFile, ec);

        std::ostringstream cmdStream;
        if (type == BuildType::MicroDOS) {
            cmdStream << compiler << " -mcpu=mosw65c02 " << opt << " -g -std=c23 -T " << linkScript
                      << " -I " << tempDir << "/include -I " << tempDir
                      << "/include/Libs -linit-stack -lcopy-data -lzero-bss " << tempDir
                      << "/Linker/App-Runtime.s " << srcFile << " -Wl,-Map=" << mapFile << " -o " << elfFile;
        } else if (type == BuildType::Assembly) {
            cmdStream << compiler << " -mcpu=mosw65c02 " << opt << " -g -T " << linkScript
                      << " -I " << tempDir << "/include -I " << tempDir
                      << "/include/Libs -nostartfiles " << srcFile << " -Wl,-Map=" << mapFile << " -o " << elfFile;
        } else {
            cmdStream << compiler << " -mcpu=mosw65c02 " << opt << " -g -T " << linkScript
                      << " -I " << tempDir << "/include -I " << tempDir
                      << "/include/Libs -flto -ffunction-sections -fdata-sections -fno-unroll-loops -fnonreentrant "
                         "-Wl,--gc-sections -Wl,--lto-O2 "
                      << srcFile << " " << tempDir << "/Linker/bios.s " << tempDir << "/include/Libs/BIOS.c";
            if (code.find("SD.h") != std::string::npos) {
                cmdStream << " " << tempDir << "/include/Libs/SD.c " << tempDir << "/include/Libs/fatfs/ff.c "
                          << tempDir << "/include/Libs/fatfs/diskio.c -D ENABLE_SD_SUPPORT=1";
            }
            cmdStream << " -Wl,-Map=" << mapFile << " -o " << elfFile;
        }
        std::string cmd = cmdStream.str();

        res.log += "> " + cmd + "\n";
        std::string stepLog;
        int exitCode = RunSysCommand(cmd, stepLog);
        res.log += stepLog;

        if (exitCode == 0 && fs::exists(elfFile) && fs::file_size(elfFile) > 0) {
            std::string binDir = fs::path(compiler).parent_path().string();
            std::string objcopy = (fs::path(binDir) / "llvm-objcopy").string();
#ifdef _WIN32
            objcopy += ".exe";
#endif
            if (!fs::exists(objcopy)) {
                objcopy = "llvm-objcopy";
            }
            std::string copyCmd = objcopy + " -O binary " + elfFile + " " + binFile;
            std::string copyLog;
            int copyResult = RunSysCommand(copyCmd, copyLog);
            res.log += copyLog;

            if (copyResult == 0 && fs::exists(binFile) && fs::file_size(binFile) > 0) {
                buildOk = true;
                res.log += "Optimization " + std::string(opt) + " succeeded.\n";
                break;
            }
        }

        if (IsSizeOverflowError(stepLog)) {
            res.log += "Optimization " + std::string(opt) + " exceeded memory. Falling back...\n";
        } else {
            res.log += "Compilation error at " + std::string(opt) + " (not a size issue). Aborting without fallback.\n";
            res.diagnostics = ParseDiagnostics(res.log);
            return res;
        }
    }

    if (!buildOk) {
        res.log += "Error: Compilation failed for all optimization levels (-Os, -Oz).\n";
        res.diagnostics = ParseDiagnostics(res.log);
        return res;
    }

    res.binary = ReadBinaryFile(binFile);
    if (res.binary.empty()) {
        res.log += "Error: Output binary is empty.\n";
        res.diagnostics = ParseDiagnostics(res.log);
        return res;
    }

    res.elfPath = elfFile;
    LoadELF(res.symbols, elfFile);
    if (GenerateDbgFromMap(mapFile, dbgFile)) {
        res.symbolFile = dbgFile;
    }

    res.success = true;
    res.log += "Compilation Successful.\n";
    res.diagnostics = ParseDiagnostics(res.log);
    return res;
}
#endif
