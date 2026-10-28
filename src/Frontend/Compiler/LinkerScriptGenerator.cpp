#ifndef TARGET_WASM
#include "LinkerScriptGenerator.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

std::string LinkerScriptGenerator::StripComments(const std::string& source) {
    std::string clean;
    clean.reserve(source.size());
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inString = false;
    bool inChar = false;

    for (size_t i = 0; i < source.size(); ++i) {
        if (inLineComment) {
            if (source[i] == '\n') {
                inLineComment = false;
                clean.push_back('\n');
            }
        } else if (inBlockComment) {
            if (source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/') {
                inBlockComment = false;
                ++i;
            }
        } else if (inString) {
            clean.push_back(source[i]);
            if (source[i] == '\\' && i + 1 < source.size()) {
                clean.push_back(source[++i]);
            } else if (source[i] == '"') {
                inString = false;
            }
        } else if (inChar) {
            clean.push_back(source[i]);
            if (source[i] == '\\' && i + 1 < source.size()) {
                clean.push_back(source[++i]);
            } else if (source[i] == '\'') {
                inChar = false;
            }
        } else {
            if (source[i] == '/' && i + 1 < source.size() && source[i + 1] == '/') {
                inLineComment = true;
                ++i;
            } else if (source[i] == '/' && i + 1 < source.size() && source[i + 1] == '*') {
                inBlockComment = true;
                ++i;
            } else {
                if (source[i] == '"') {
                    inString = true;
                } else if (source[i] == '\'') {
                    inChar = true;
                }
                clean.push_back(source[i]);
            }
        }
    }
    return clean;
}

CompilerFrontend::HardwareRequirements LinkerScriptGenerator::DetectHardwareRequirements(const std::string& code) {
    std::string cleanCode = StripComments(code);
    CompilerFrontend::HardwareRequirements hw;
    hw.gpu = cleanCode.find("--gpu") != std::string::npos || cleanCode.find("GPU.h") != std::string::npos ||
             cleanCode.find("--double-buffer") != std::string::npos ||
             cleanCode.find("GPUDoubleBuffer.h") != std::string::npos;
    hw.sid = cleanCode.find("--sid") != std::string::npos || cleanCode.find("SID.h") != std::string::npos;
    hw.net = cleanCode.find("--net") != std::string::npos || cleanCode.find("NET.h") != std::string::npos;
    hw.sd = cleanCode.find("--sd") != std::string::npos || cleanCode.find("SD.h") != std::string::npos;
    return hw;
}

std::string LinkerScriptGenerator::GenerateLinkerScript(const std::string& code) {
    std::string cleanCode = StripComments(code);
    bool hasGpu = cleanCode.find("--gpu") != std::string::npos || cleanCode.find("GPU.h") != std::string::npos;
    bool hasDoubleBuffer =
        cleanCode.find("--double-buffer") != std::string::npos || cleanCode.find("GPUDoubleBuffer.h") != std::string::npos;
    bool hasNet = cleanCode.find("--net") != std::string::npos || cleanCode.find("NET.h") != std::string::npos;
    bool hasSid = cleanCode.find("--sid") != std::string::npos || cleanCode.find("SID.h") != std::string::npos;

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

std::string LinkerScriptGenerator::GenerateAppLinkerScript(bool hasGpu) {
    std::ostringstream ld;
    ld << "/* Auto-generated link.ld for microDOS apps with LLVM-MOS */\n\n";
    ld << "__rc0 = 0x00;\n";
    ld << "INCLUDE imag-regs.ld\n";
    ld << "ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")\n";
    ld << "ASSERT(__rc31 <= 0x5F, \"Registros virtuales exceden Zero Page de app.\")\n\n";
    ld << "MEMORY {\n";
    ld << "    zp   (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x60 - (__rc31 + 1)\n";
    if (hasGpu) {
        ld << "    core (rx) : ORIGIN = 0x0200, LENGTH = 0x3E00\n";
        ld << "    bss  (rw) : ORIGIN = 0x4000, LENGTH = 0x1000\n";
    } else {
        ld << "    core (rx) : ORIGIN = 0x0200, LENGTH = 0x4E00\n";
        ld << "    bss  (rw) : ORIGIN = 0x5000, LENGTH = 0x2000\n";
    }
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
    ld << "__stack = 0x" << (hasGpu ? "4FFF" : "6FFF") << ";\n\n";
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

#endif
