#include "IDEWindow.h"
#ifndef EMSCRIPTEN
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

void DrawIDEWindow(Control::AppState& state) {
    if (!state.ide.open) return;

#ifdef EMSCRIPTEN
    // IDE is not supported in WebAssembly
    state.ide.open = false;
    return;
#else

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CC65 Code Editor", &state.ide.open)) {
        
        // Toolbar
        if (ImGui::RadioButton("C", state.ide.isCMode)) state.ide.isCMode = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("Assembly", !state.ide.isCMode)) state.ide.isCMode = false;

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        if (ImGui::Button("Compile & Run", ImVec2(120, 0))) {
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
        static char codeBuffer[65536];
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

    }
    ImGui::End();
#endif
}

}
