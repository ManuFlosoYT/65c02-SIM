#include "Frontend/GUI/IDEBuildDeployer.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/IDEDocumentController.h"
#include "Frontend/GUI/CustomFileDialog.h"

#include <filesystem>
#include <fstream>

namespace GUI {

bool IDEBuildDeployer::SaveCurrentFileForCompile(Control::AppState& state) {
    state.ide.outputLog.clear();
    auto& tabs = IDEDocumentController::GetTabs();
    auto& activeIndex = IDEDocumentController::GetActiveTabIndex();
    if (activeIndex >= tabs.size() || tabs.at(activeIndex).isReadOnly) {
        state.ide.outputLog = "Error: No active source file to compile.";
        return false;
    }
    state.ide.code = IDEDocumentController::GetSanitizedTabCode(tabs.at(activeIndex));
    state.ide.currentFilePath = tabs.at(activeIndex).filePath;
    if (state.ide.currentFilePath.empty()) {
        state.ide.outputLog = "Error: Please save your file before compiling.";
        return false;
    }
    std::ofstream fileOut(state.ide.currentFilePath);
    if (!fileOut) {
        state.ide.outputLog = "Error: Failed to auto-save file before compiling.";
        return false;
    }
    fileOut << state.ide.code;
    return true;
}

#ifndef TARGET_WASM
CompilerFrontend::BuildResult IDEBuildDeployer::CompileCurrentFile(Control::AppState& state) {
    CompilerFrontend::BuildType type = CompilerFrontend::BuildType::C;
    if (state.ide.mode == Control::IDEMode::ASM) {
        type = CompilerFrontend::BuildType::Assembly;
    } else if (state.ide.mode == Control::IDEMode::MicroDOS) {
        type = CompilerFrontend::BuildType::MicroDOS;
    }
    auto result = CompilerFrontend::Compile(type, state.ide.code, state.ide.currentFilePath);
    state.ide.outputLog = result.log;
    IDEDocumentController::GetLastDiagnostics() = result.diagnostics;
    IDEDocumentController::ApplyDiagnosticsToTabs(result.diagnostics);
    if (!result.success && !result.diagnostics.empty()) {
        IDEDocumentController::JumpToDiagnostic(result.diagnostics.front());
    }
    return result;
}

void IDEBuildDeployer::HandleCompileAndRun(Control::AppState& state) {
    if (!SaveCurrentFileForCompile(state)) {
        return;
    }

    auto result = CompileCurrentFile(state);

    if (result.success && !result.binary.empty()) {
        if (state.ide.mode == Control::IDEMode::MicroDOS) {
            std::filesystem::path filePath(state.ide.currentFilePath);
            std::string appName = filePath.stem().string();
            if (appName.empty()) {
                appName = "app";
            }
            std::error_code ec;
            std::filesystem::create_directories("output/apps", ec);
            std::string targetApp = "output/apps/" + appName + ".app";
            std::ofstream out(targetApp, std::ios::binary);
            if (out) {
                for (const auto byte : result.binary) {
                    out.put(static_cast<char>(byte));
                }
                if (result.symbols.IsLoaded()) {
                    state.rom.symbols = std::move(result.symbols);
                } else if (!result.symbolFile.empty() && std::filesystem::exists(result.symbolFile)) {
                    state.rom.symbols.Clear();
                    state.rom.symbols.LoadFromFile(result.symbolFile);
                }
                state.ide.outputLog += "\nMicroDOS app built successfully: " + targetApp;
                if (state.emulator.GetSDCard().IsMounted() && state.emulator.GetSDCard().IsFolderMount()) {
                    state.emulator.GetSDCard().SyncFromHost();
                    state.ide.outputLog += "\nSD Card synchronized with host folder.";
                }
            } else {
                state.ide.outputLog += "\nFailed to write MicroDOS app: " + targetApp;
            }
        } else {
            state.emulator.Pause();
            state.emulator.SetCartridge(Core::Cartridge{});
            state.emulation.gpuEnabled = result.hardware.gpu;
            state.emulator.SetGPUEnabled(result.hardware.gpu);
            state.emulator.GetSID().EnableSound(result.hardware.sid);
            state.emulator.SetSIDEnabled(result.hardware.sid);
            state.emulation.espEnabled = result.hardware.net;
            state.emulator.SetESPEnabled(result.hardware.net);
            state.emulation.sdEnabled = result.hardware.sd;
            state.emulator.SetSDEnabled(result.hardware.sd);
            std::string errorMsg;
            if (state.emulator.InitFromMemory(result.binary, "compiled.bin", errorMsg)) {
                state.rom.bin = "compiled.bin";
                state.rom.data = result.binary;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                bool symbolsLoaded = false;
                if (result.symbols.IsLoaded()) {
                    state.rom.symbols = std::move(result.symbols);
                    symbolsLoaded = true;
                } else if (!result.symbolFile.empty() && std::filesystem::exists(result.symbolFile)) {
                    symbolsLoaded = state.rom.symbols.LoadFromFile(result.symbolFile);
                }
                if (!symbolsLoaded && !state.ide.currentFilePath.empty()) {
                    std::filesystem::path srcPath(state.ide.currentFilePath);
                    std::string stem = srcPath.stem().string();
                    const std::vector<std::filesystem::path> dbgCandidates = {
                        srcPath.parent_path() / (stem + ".dbg"),
                        srcPath.parent_path() / "build" / (stem + ".dbg"),
                        srcPath.parent_path().parent_path() / "build" / (stem + ".dbg"),
                        std::filesystem::path("output/rom") / (stem + ".dbg"),
                        std::filesystem::path("sdk/src/build") / (stem + ".dbg")
                    };
                    for (const auto& candidate : dbgCandidates) {
                        if (std::filesystem::exists(candidate)) {
                            if (state.rom.symbols.LoadFromFile(candidate.string())) {
                                symbolsLoaded = true;
                                break;
                            }
                        }
                    }
                }
                if (symbolsLoaded) {
                    state.ide.outputLog += "\nLoaded " + std::to_string(state.rom.symbols.GetSymbolCount()) +
                                           " debug symbols into Disassembly.";
                }
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                state.emulator.ClearProfiler();
                state.emulator.Resume();
                state.ide.outputLog += "\nSuccessfully loaded to emulator and Reset.";
            } else {
                state.ide.outputLog += "\nEmulator Load Error: " + errorMsg;
            }
        }
    }
}

void IDEBuildDeployer::HandleCompileAndExport(Control::AppState& state) {
    if (!SaveCurrentFileForCompile(state)) {
        return;
    }

    auto result = CompileCurrentFile(state);

    if (result.success && !result.binary.empty()) {
        state.ide.exportBinary = result.binary;
        if (state.ide.mode == Control::IDEMode::MicroDOS) {
            state.ide.outputLog += "\nCompilation successful. Choose where to save the .app file.";
            Frontend::CustomFileDialog::OpenDialog("IDE_ExportAppDlg", "Export MicroDOS App As", ".app", ".");
        } else {
            state.ide.outputLog += "\nCompilation successful. Choose where to save the .bin file.";
            Frontend::CustomFileDialog::OpenDialog("IDE_ExportBinDlg", "Export Binary As", ".bin", ".");
        }
    }
}
#endif

bool IDEBuildDeployer::ExportBinary(Control::AppState& state, const std::string& exportPath) {
    std::ofstream fileOut(exportPath, std::ios::binary);
    if (fileOut && !state.ide.exportBinary.empty()) {
        for (const auto byte : state.ide.exportBinary) {
            fileOut.put(static_cast<char>(byte));
        }
        state.ide.outputLog += "\nExported binary to " + exportPath;
        state.ide.exportBinary.clear();
        return true;
    }
    state.ide.outputLog += "\nFailed to export binary to " + exportPath;
    state.ide.exportBinary.clear();
    return false;
}

bool IDEBuildDeployer::ExportApp(Control::AppState& state, const std::string& exportPath) {
    std::ofstream fileOut(exportPath, std::ios::binary);
    if (fileOut && !state.ide.exportBinary.empty()) {
        for (const auto byte : state.ide.exportBinary) {
            fileOut.put(static_cast<char>(byte));
        }
        state.ide.outputLog += "\nExported app to " + exportPath;
        state.ide.exportBinary.clear();
        return true;
    }
    state.ide.outputLog += "\nFailed to export app to " + exportPath;
    state.ide.exportBinary.clear();
    return false;
}

} // namespace GUI
