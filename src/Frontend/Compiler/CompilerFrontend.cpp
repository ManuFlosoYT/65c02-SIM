#ifndef TARGET_WASM
#include "CompilerFrontend.h"
#include "CompilerVFS.h"
#include "ToolchainLocator.h"
#include "ProcessRunner.h"
#include "LinkerScriptGenerator.h"
#include "CompilerDiagnosticParser.h"
#include "ElfSymbolExtractor.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

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

int CompilerFrontend::RunCommand(const std::string& cmd, std::string& output) {
    return ProcessRunner::RunCommand(cmd, output);
}

bool CompilerFrontend::LoadELF(Control::SymbolTable& table, const std::string& elfPath) {
    return ElfSymbolExtractor::LoadELF(table, elfPath);
}

std::vector<CompilerFrontend::Diagnostic> CompilerFrontend::ParseDiagnostics(const std::string& log) {
    return CompilerDiagnosticParser::ParseDiagnostics(log);
}

std::string CompilerFrontend::GenerateLinkerScript(const std::string& code) {
    return LinkerScriptGenerator::GenerateLinkerScript(code);
}

std::string CompilerFrontend::GenerateAppLinkerScript() {
    return LinkerScriptGenerator::GenerateAppLinkerScript();
}

CompilerFrontend::BuildResult CompilerFrontend::Compile(BuildType type, const std::string& code,
                                                        const std::string& sourceName) {
    BuildResult res;
    if (code.empty()) {
        res.log = "Error: Empty source code.\n";
        res.diagnostics = ParseDiagnostics(res.log);
        return res;
    }

    res.hardware = LinkerScriptGenerator::DetectHardwareRequirements(code);

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
        WriteTextFile(linkScript, LinkerScriptGenerator::GenerateAppLinkerScript());
    } else {
        WriteTextFile(linkScript, LinkerScriptGenerator::GenerateLinkerScript(code));
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
            cmdStream << "\"" << compiler << "\" -mcpu=mosw65c02 " << opt << " -g -std=c23 -T \"" << linkScript
                      << "\" -I \"" << tempDir << "/include\" -I \"" << tempDir
                      << "/include/Libs\" -linit-stack -lcopy-data -lzero-bss \"" << tempDir
                      << "/Linker/App-Runtime.s\" \"" << srcFile << "\" \"-Wl,-Map=" << mapFile << "\" -o \"" << elfFile << "\"";
        } else if (type == BuildType::Assembly) {
            cmdStream << "\"" << compiler << "\" -mcpu=mosw65c02 " << opt << " -g -T \"" << linkScript
                      << "\" -I \"" << tempDir << "/include\" -I \"" << tempDir
                      << "/include/Libs\" -nostartfiles \"" << srcFile << "\" \"-Wl,-Map=" << mapFile << "\" -o \"" << elfFile << "\"";
        } else {
            cmdStream << "\"" << compiler << "\" -mcpu=mosw65c02 " << opt << " -g -T \"" << linkScript
                      << "\" -I \"" << tempDir << "/include\" -I \"" << tempDir
                      << "/include/Libs\" -flto -ffunction-sections -fdata-sections -fno-unroll-loops -fnonreentrant "
                         "-Wl,--gc-sections -Wl,--lto-O2 \""
                      << srcFile << "\" \"" << tempDir << "/Linker/bios.s\" \"" << tempDir << "/include/Libs/BIOS.c\"";
            if (res.hardware.sd) {
                cmdStream << " \"" << tempDir << "/include/Libs/SD.c\" \"" << tempDir << "/include/Libs/fatfs/ff.c\" \""
                          << tempDir << "/include/Libs/fatfs/diskio.c\" -D ENABLE_SD_SUPPORT=1";
            }
            cmdStream << " \"-Wl,-Map=" << mapFile << "\" -o \"" << elfFile << "\"";
        }
        std::string cmd = cmdStream.str();

        res.log += "> " + cmd + "\n";
        std::string stepLog;
        int exitCode = ProcessRunner::RunCommand(cmd, stepLog);
        res.log += stepLog;

        if (exitCode == 0 && fs::exists(elfFile) && fs::file_size(elfFile) > 0) {
            std::string objcopy = ToolchainLocator::FindObjcopy();
            std::string copyCmd = "\"" + objcopy + "\" -O binary \"" + elfFile + "\" \"" + binFile + "\"";
            std::string copyLog;
            int copyResult = ProcessRunner::RunCommand(copyCmd, copyLog);
            res.log += copyLog;

            if (copyResult == 0 && fs::exists(binFile) && fs::file_size(binFile) > 0) {
                buildOk = true;
                res.log += "Optimization " + std::string(opt) + " succeeded.\n";
                break;
            }
        }

        if (CompilerDiagnosticParser::IsSizeOverflowError(stepLog)) {
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
    if (ElfSymbolExtractor::GenerateDbgFromMap(mapFile, dbgFile)) {
        res.symbolFile = dbgFile;
    }

    res.success = true;
    res.log += "Compilation Successful.\n";
    res.diagnostics = ParseDiagnostics(res.log);
    return res;
}
#endif
