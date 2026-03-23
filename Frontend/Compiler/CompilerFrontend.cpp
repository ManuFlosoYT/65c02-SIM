#ifndef EMSCRIPTEN
#include "CompilerFrontend.h"
#include "CC65VFS.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::vector<uint8_t> ReadBinaryFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return {};
    auto size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> data((size_t)size);
    in.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

static void WriteTextFile(const std::string& path, const std::string& txt) {
    std::ofstream out(path, std::ios::binary);
    if (out) out << txt;
}

static int RunSysCommand(const std::string& cmd, std::string& log) {
    std::string tempLog = CC65VFS::GetTempDir() + "/temp.log";
    std::string fullCmd = cmd + " > " + tempLog + " 2>&1";
    
    int result = std::system(fullCmd.c_str());
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

    if (hasGpu || hasDoubleBuffer) res.push_back({0x2000, 0x3FFF, "GPU VRAM Front Buffer"});
    if (hasDoubleBuffer) res.push_back({0x6010, 0x790F, "GPU VRAM Back Buffer"});
    
    std::sort(res.begin(), res.end(), [](const Block& a, const Block& b){ return a.start < b.start; });
    std::vector<Block> merged;
    for (auto& b : res) {
        if (merged.empty()) merged.push_back(b);
        else {
            auto& prev = merged.back();
            if (b.start <= prev.end + 1) {
                prev.end = std::max(prev.end, b.end);
                prev.name += " + " + b.name;
            } else {
                merged.push_back(b);
            }
        }
    }

    std::vector<Block> ram;
    uint32_t current = 0x0400; // ram_start
    uint32_t ram_end = 0x8000;
    for (auto& b : merged) {
        if (current < b.start) ram.push_back({current, b.start - 1, ""});
        current = std::max(current, b.end + 1);
    }
    if (current < ram_end) ram.push_back({current, ram_end - 1, ""});

    std::ostringstream cfg;
    cfg << "MEMORY {\n";
    cfg << "    ZP:           start = $0000, size = $0100, type = rw, define = yes, file = \"\";\n";
    cfg << "    INPUT_BUFFER: start = $0300, size = $0100, type = rw, file = \"\";\n";
    
    for (size_t i = 0; i < ram.size(); i++) {
        uint32_t size = ram[i].end - ram[i].start + 1;
        std::string name = "RAM_" + std::to_string(i + 1);
        cfg << "    " << name << ": start = $" << std::hex << ram[i].start << ", size = $" << size << ", type = rw, define = yes, file = \"\";\n";
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

CompilerFrontend::BuildResult CompilerFrontend::Compile(BuildType type, const std::string& code) {
    BuildResult res;
    std::string tempDir = CC65VFS::GetTempDir();
    if (tempDir.empty()) {
        res.log = "Error: CC65 VFS not initialized.";
        return res;
    }

    std::string srcFile = tempDir + (type == BuildType::C ? "/test.c" : "/test.s");
    WriteTextFile(srcFile, code);

#ifdef _WIN32
    std::string ext = ".exe";
#else
    std::string ext = "";
#endif

    std::string cc65 = tempDir + "/cc65" + ext;
    std::string ca65 = tempDir + "/ca65" + ext;
    std::string ld65 = tempDir + "/ld65" + ext;
    
    if (type == BuildType::C) {
        // cc65
        std::string cmd = cc65 + " -I " + tempDir + "/include -O --cpu 65C02 -g " + srcFile + " -o " + tempDir + "/test.s";
        res.log += "> " + cmd + "\n";
        if (RunSysCommand(cmd, res.log) != 0) return res;
    }

    // Assemble test.s -> test.o
    {
        std::string cmd = ca65 + " -g -t none " + tempDir + "/test.s -o " + tempDir + "/test.o";
        res.log += "> " + cmd + "\n";
        if (RunSysCommand(cmd, res.log) != 0) return res;
    }

    // Assemble Linker files
    {
        std::string cmd1 = ca65 + " -g -t none " + tempDir + "/Linker/bios.s -o " + tempDir + "/bios.o";
        res.log += "> " + cmd1 + "\n";
        if (RunSysCommand(cmd1, res.log) != 0) return res;

        if (type == BuildType::C) {
            std::string cmd2 = ca65 + " -g -t none " + tempDir + "/Linker/C-Runtime.s -o " + tempDir + "/cr.o";
            res.log += "> " + cmd2 + "\n";
            if (RunSysCommand(cmd2, res.log) != 0) return res;
        }
    }

    // FatFS (SD.h)
    std::string extraObjs = "";
    if (type == BuildType::C && code.find("SD.h") != std::string::npos) {
        // Compile SD.c, ff.c, diskio.c
        const char* files[] = { "Libs/SD.c", "Libs/fatfs/ff.c", "Libs/fatfs/diskio.c" };
        const char* objNames[] = { "sd", "ff", "diskio" };
        for (int i = 0; i < 3; ++i) {
            std::string s_cmd = cc65 + " -I " + tempDir + "/include -O --cpu 65C02 " + tempDir + "/include/" + files[i] + " -o " + tempDir + "/" + objNames[i] + ".s";
            res.log += "> " + s_cmd + "\n";
            if (RunSysCommand(s_cmd, res.log) != 0) return res;

            std::string o_cmd = ca65 + " -g -t none " + tempDir + "/" + objNames[i] + ".s -o " + tempDir + "/" + objNames[i] + ".o";
            res.log += "> " + o_cmd + "\n";
            if (RunSysCommand(o_cmd, res.log) != 0) return res;

            extraObjs += tempDir + "/" + objNames[i] + ".o ";
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
        objs += tempDir + "/test.o ";
        objs += extraObjs;
        
        std::string cmd = ld65 + " -C " + cfgFile + " -o " + tempDir + "/out.bin " + objs;
        if (type == BuildType::C) {
            cmd += " " + tempDir + "/lib/none.lib";
        }
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
