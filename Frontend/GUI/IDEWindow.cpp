#include "IDEWindow.h"
#include <ImGuiFileDialog.h>
#include <fstream>
#include <sstream>

#ifndef EMSCRIPTEN
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

static char codeBuffer[65536] = {0};

void DrawIDEWindow(Control::AppState& state) {
    if (!state.ide.open) return;

#ifdef EMSCRIPTEN
    // IDE is not supported in WebAssembly
    state.ide.open = false;
    return;
#else

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CC65 Code Editor", &state.ide.open)) {
        
        // Toolbar: File Operations
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
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (!state.ide.currentFilePath.empty()) {
            ImGui::Text("File: %s", state.ide.currentFilePath.c_str());
        } else {
            ImGui::Text("File: (Unsaved)");
        }

        ImGui::Separator();

        // Toolbar: Compiler
        if (ImGui::RadioButton("C", state.ide.isCMode)) { state.ide.isCMode = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Assembly", !state.ide.isCMode)) state.ide.isCMode = false;

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        if (ImGui::Button("Compile & Run", ImVec2(180, 0))) {
            state.ide.outputLog.clear();
            if (state.ide.code.empty()) {
                state.ide.outputLog = "Error: Code is empty.";
            } else {
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
                        state.rom.symbols.Clear(); // Can load from .map or .lbl if parsed in the future
                        state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                        state.emulator.ClearProfiler();
                        state.ide.outputLog += "\nSuccessfully loaded to emulator and Reset.";
                    } else {
                        state.ide.outputLog += "\nEmulator Load Error: " + errorMsg;
                    }
                }
            }
        }

        ImGui::Separator();

        // Editor and Output Split
        float outputHeight = 150.0f;
        float h = ImGui::GetContentRegionAvail().y;

        // Code Editor
        ImGui::Text("Code Editor:");
        // Ensure string buffer is large enough for editing
        if (state.ide.code.capacity() < 65536) state.ide.code.reserve(65536);
        state.ide.code.resize(strlen(state.ide.code.c_str())); 
        // Note: Using a fixed buffer or a callback for std::string resizing
        // The simplest way with ImGui InputTextMultiline is to pass a statically large buffer or use a callback
        if (codeBuffer[0] == '\0' && !state.ide.code.empty()) {
            strncpy(codeBuffer, state.ide.code.c_str(), sizeof(codeBuffer) - 1);
        }
        
        if (ImGui::InputTextMultiline("##CodeEditor", codeBuffer, sizeof(codeBuffer), 
            ImVec2(-FLT_MIN, h - outputHeight - 30), 
            ImGuiInputTextFlags_AllowTabInput)) {
            state.ide.code = codeBuffer;
        }

        // External Output Log
        ImGui::Text("Build Output:");
        ImGui::InputTextMultiline("##BuildOutput", (char*)state.ide.outputLog.c_str(), state.ide.outputLog.capacity() + 1, 
            ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

        // File Dialogs
        if (ImGuiFileDialog::Instance()->Display("IDE_OpenFile", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                state.ide.currentFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                std::ifstream in(state.ide.currentFilePath);
                if (in) {
                    std::ostringstream ss;
                    ss << in.rdbuf();
                    state.ide.code = ss.str();
                    strncpy(codeBuffer, state.ide.code.c_str(), sizeof(codeBuffer) - 1);
                    codeBuffer[sizeof(codeBuffer) - 1] = '\0';
                    state.ide.outputLog = "Opened " + state.ide.currentFilePath;
                    
                    // Auto-detect mode based on extension
                    if (state.ide.currentFilePath.ends_with(".c")) state.ide.isCMode = true;
                    else if (state.ide.currentFilePath.ends_with(".s") || state.ide.currentFilePath.ends_with(".asm")) state.ide.isCMode = false;
                } else {
                    state.ide.outputLog = "Failed to open " + state.ide.currentFilePath;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("IDE_SaveFile", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                state.ide.currentFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                std::ofstream out(state.ide.currentFilePath);
                if (out) {
                    out << state.ide.code;
                    state.ide.outputLog = "Saved " + state.ide.currentFilePath;
                } else {
                    state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

    }
    ImGui::End();
#endif
}

}
