#include "IDEWindow.h"
#include <ImGuiFileDialog.h>
#include <fstream>
#include <sstream>
#include <array>

#ifndef EMSCRIPTEN
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

static void DrawFileToolbar(Control::AppState& state) {
    if (ImGui::Button("Open File")) {
        ImGuiFileDialog::Instance()->OpenDialog("IDE_OpenFile", "Open Source File", ".c,.s,.asm,.*", ".");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        if (state.ide.currentFilePath.empty()) {
            ImGuiFileDialog::Instance()->OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
        } else {
            std::ofstream out(state.ide.currentFilePath);
            if (out) {
                out << state.ide.code;
                state.ide.outputLog = "Saved " + state.ide.currentFilePath;
            } else {
                state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As")) {
        ImGuiFileDialog::Instance()->OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
    }
    
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();

    if (!state.ide.currentFilePath.empty()) {
        std::string label = "File: " + state.ide.currentFilePath;
        ImGui::TextUnformatted(label.c_str());
    } else {
        ImGui::TextUnformatted("File: (Unsaved)");
    }
}

static void HandleCompileAndRun(Control::AppState& state) {
    state.ide.outputLog.clear();
    if (state.ide.code.empty()) {
        state.ide.outputLog = "Error: Code is empty.";
        return;
    }
#ifndef EMSCRIPTEN
    CompilerFrontend::BuildType type = state.ide.isCMode ? 
        CompilerFrontend::BuildType::C : CompilerFrontend::BuildType::Assembly;
    
    auto result = CompilerFrontend::Compile(type, state.ide.code);
    state.ide.outputLog = result.log;

    if (result.success && !result.binary.empty()) {
        state.emulator.Pause();
        std::string errorMsg;
        if (state.emulator.InitFromMemory(result.binary.data(), result.binary.size(), "compiled.bin", errorMsg)) {
            state.rom.bin = "compiled.bin";
            state.rom.data = result.binary;
            state.rom.loaded = true;
            state.rom.symbols.Clear();
            state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
            state.emulator.ClearProfiler();
            state.ide.outputLog += "\nSuccessfully loaded to emulator and Reset.";
        } else {
            state.ide.outputLog += "\nEmulator Load Error: " + errorMsg;
        }
    }
#endif
}

static void HandleCompileAndExport(Control::AppState& state) {
    state.ide.outputLog.clear();
    if (state.ide.code.empty()) {
        state.ide.outputLog = "Error: Code is empty.";
        return;
    }
#ifndef EMSCRIPTEN
    CompilerFrontend::BuildType type = state.ide.isCMode ? 
        CompilerFrontend::BuildType::C : CompilerFrontend::BuildType::Assembly;
    
    auto result = CompilerFrontend::Compile(type, state.ide.code);
    state.ide.outputLog = result.log;

    if (result.success && !result.binary.empty()) {
        state.ide.exportBinary = result.binary;
        state.ide.outputLog += "\nCompilation successful. Choose where to save the .bin file.";
        ImGuiFileDialog::Instance()->OpenDialog("IDE_ExportBinDlg", "Export Binary As", ".bin", ".");
    }
#endif
}

static void DrawCompilerToolbar(Control::AppState& state) {
    if (ImGui::RadioButton("C", state.ide.isCMode)) {
        state.ide.isCMode = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Assembly", !state.ide.isCMode)) {
        state.ide.isCMode = false;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::Button("Compile & Run", ImVec2(180.0F, 0.0F))) {
        HandleCompileAndRun(state);
    }

    ImGui::SameLine();
    if (ImGui::Button("Compile & Export", ImVec2(180.0F, 0.0F))) {
        HandleCompileAndExport(state);
    }
}

static void DrawToolbar(Control::AppState& state) {
    DrawFileToolbar(state);
    ImGui::Separator();
    DrawCompilerToolbar(state);
}

static void HandleOpenFile(Control::AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("IDE_OpenFile", ImGuiWindowFlags_NoCollapse, ImVec2(700.0F, 400.0F))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            state.ide.currentFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::ifstream fileIn(state.ide.currentFilePath);
            if (fileIn) {
                std::ostringstream stringStream;
                stringStream << fileIn.rdbuf();
                state.ide.code = stringStream.str();
                
                size_t copySize = std::min(state.ide.code.size(), state.ide.codeBuffer.size() - 1);
                std::copy_n(state.ide.code.begin(), copySize, state.ide.codeBuffer.begin());
                state.ide.codeBuffer.at(copySize) = '\0';

                state.ide.outputLog = "Opened " + state.ide.currentFilePath;
                
                if (state.ide.currentFilePath.ends_with(".c")) {
                    state.ide.isCMode = true;
                } else if (state.ide.currentFilePath.ends_with(".s") || state.ide.currentFilePath.ends_with(".asm")) {
                    state.ide.isCMode = false;
                }
            } else {
                state.ide.outputLog = "Failed to open " + state.ide.currentFilePath;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleSaveFile(Control::AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("IDE_SaveFile", ImGuiWindowFlags_NoCollapse, ImVec2(700.0F, 400.0F))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            state.ide.currentFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::ofstream fileOut(state.ide.currentFilePath);
            if (fileOut) {
                fileOut << state.ide.code;
                state.ide.outputLog = "Saved " + state.ide.currentFilePath;
            } else {
                state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleExportFile(Control::AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("IDE_ExportBinDlg", ImGuiWindowFlags_NoCollapse, ImVec2(700.0F, 400.0F))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string exportPath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::ofstream fileOut(exportPath, std::ios::binary);
            if (fileOut && !state.ide.exportBinary.empty()) {
                fileOut.write(reinterpret_cast<const char*>(state.ide.exportBinary.data()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                            static_cast<std::streamsize>(state.ide.exportBinary.size()));
                state.ide.outputLog += "\nExported binary to " + exportPath;
            } else {
                state.ide.outputLog += "\nFailed to export binary to " + exportPath;
            }
            state.ide.exportBinary.clear();
        } else {
            state.ide.exportBinary.clear();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleIDEFileDialogs(Control::AppState& state) {
    HandleOpenFile(state);
    HandleSaveFile(state);
    HandleExportFile(state);
}

void DrawIDEWindow(Control::AppState& state) {
    if (!state.ide.open) {
        return;
    }

#ifdef EMSCRIPTEN
    state.ide.open = false;
    return;
#else
    ImGui::SetNextWindowSize(ImVec2(800.0F, 600.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CC65 Code Editor", &state.ide.open)) {
        DrawToolbar(state);
        ImGui::Separator();

        float outputHeight = 150.0F;
        float availableHeight = ImGui::GetContentRegionAvail().y;

        ImGui::TextUnformatted("Code Editor:");
        if (state.ide.code.capacity() < 65536) {
            state.ide.code.reserve(65536);
        }
        
        if (state.ide.codeBuffer.at(0) == '\0' && !state.ide.code.empty()) {
            size_t copySize = std::min(state.ide.code.size(), state.ide.codeBuffer.size() - 1);
            std::copy_n(state.ide.code.begin(), copySize, state.ide.codeBuffer.begin());
            state.ide.codeBuffer.at(copySize) = '\0';
        }
        
        if (ImGui::InputTextMultiline("##CodeEditor", state.ide.codeBuffer.data(), state.ide.codeBuffer.size(), 
            ImVec2(-FLT_MIN, availableHeight - outputHeight - 30.0F), 
            ImGuiInputTextFlags_AllowTabInput)) {
            state.ide.code = state.ide.codeBuffer.data();
        }

        ImGui::TextUnformatted("Build Output:");
        static std::vector<char> outputBuffer;
        outputBuffer.assign(state.ide.outputLog.begin(), state.ide.outputLog.end());
        outputBuffer.push_back('\0');
        
        ImGui::InputTextMultiline("##BuildOutput", outputBuffer.data(), 
            outputBuffer.size(), 
            ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

        HandleIDEFileDialogs(state);
    }
    ImGui::End();
#endif
}

}
