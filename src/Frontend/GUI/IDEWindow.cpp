#include "IDEWindow.h"
#include <bit>
#include "Frontend/UI/CustomFileDialog.h"
#include <fstream>
#include <sstream>
#include <array>
#include <TextEditor.h>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

using Frontend::CustomFileDialog;

namespace {
    TextEditor& GetTextEditor() {
        static TextEditor editor;
        return editor;
    }

    bool& IsEditorInitialized() {
        static bool initialized = false;
        return initialized;
    }

    TextEditor::LanguageDefinition GetCC65LanguageDefinition() {
        TextEditor::LanguageDefinition langDef = TextEditor::LanguageDefinition::CPlusPlus();
        langDef.mName = "6502 Assembly";

        static const std::array<const char*, 64> opcodes = {{
            "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk", "bvc", "bvs", "clc",
            "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx", "iny", "jmp",
            "jsr", "lda", "ldx", "ldy", "lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti",
            "rts", "sbc", "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
            "bra", "phx", "phy", "plx", "ply", "stz", "trb", "tsb" // 65C02 instructions
        }};

        static const std::array<const char*, 28> directives = {{
            ".byte", ".word", ".dword", ".res", ".text", ".data", ".bss", ".rodata", ".org", ".include", ".incbin",
            ".macpack", ".macro", ".endmacro", ".proc", ".endproc", ".scope", ".endscope", ".out", ".segment", ".export",
            ".import", ".importzp", ".exportzp", ".global", ".globalzp", ".align", ".addr"
        }};

        for (const auto& keyword : opcodes) {
            langDef.mKeywords.insert(keyword);
        }
        for (const auto& keyword : directives) {
            langDef.mKeywords.insert(keyword);
        }

        langDef.mSingleLineComment = ";";
        langDef.mCommentStart = "/*";
        langDef.mCommentEnd = "*/";

        langDef.mCaseSensitive = false;
        langDef.mAutoIndentation = true;

        return langDef;
    }

    void InitEditor(Control::AppState& state) {
        if (!IsEditorInitialized()) {
            GetTextEditor().SetPalette(TextEditor::GetDarkPalette());
            GetTextEditor().SetLanguageDefinition(state.ide.isCMode ? TextEditor::LanguageDefinition::CPlusPlus() : GetCC65LanguageDefinition());
            IsEditorInitialized() = true;
        }
    }
}

static void DrawFileToolbar(Control::AppState& state) {
    if (ImGui::Button("Open File")) {
        CustomFileDialog::OpenDialog("IDE_OpenFile", "Open Source File", ".c,.s,.asm,.*", ".");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        if (state.ide.currentFilePath.empty()) {
            CustomFileDialog::OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
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
        CustomFileDialog::OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
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
    if (state.ide.currentFilePath.empty()) {
        state.ide.outputLog = "Error: Please save your file before compiling.";
        return;
    }
    std::ofstream fileOut(state.ide.currentFilePath);
    if (fileOut) {
        fileOut << state.ide.code;
    } else {
        state.ide.outputLog = "Error: Failed to auto-save file before compiling.";
        return;
    }

#ifndef TARGET_WASM
    CompilerFrontend::BuildType type = state.ide.isCMode ? 
        CompilerFrontend::BuildType::C : CompilerFrontend::BuildType::Assembly;
    
    auto result = CompilerFrontend::Compile(type, state.ide.code, state.ide.currentFilePath);
    state.ide.outputLog = result.log;

    if (result.success && !result.binary.empty()) {
        state.emulator.Pause();
        std::string errorMsg;
        if (state.emulator.InitFromMemory(result.binary, "compiled.bin", errorMsg)) {
            state.rom.bin = "compiled.bin";
            state.rom.data = result.binary;
            state.rom.loaded = true;
            state.rom.symbols.Clear();
            if (!result.dbgFile.empty()) {
                if (state.rom.symbols.LoadFromFile(result.dbgFile)) {
                    state.ide.outputLog += "\nLoaded debug symbols.";
                } else {
                    state.ide.outputLog += "\nFailed to load debug symbols.";
                }
            }
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
    if (state.ide.currentFilePath.empty()) {
        state.ide.outputLog = "Error: Please save your file before compiling.";
        return;
    }
    std::ofstream fileOut(state.ide.currentFilePath);
    if (fileOut) {
        fileOut << state.ide.code;
    } else {
        state.ide.outputLog = "Error: Failed to auto-save file before compiling.";
        return;
    }

#ifndef TARGET_WASM
    CompilerFrontend::BuildType type = state.ide.isCMode ? 
        CompilerFrontend::BuildType::C : CompilerFrontend::BuildType::Assembly;
    
    auto result = CompilerFrontend::Compile(type, state.ide.code, state.ide.currentFilePath);
    state.ide.outputLog = result.log;

    if (result.success && !result.binary.empty()) {
        state.ide.exportBinary = result.binary;
        state.ide.exportDbgPath = result.dbgFile;
        state.ide.outputLog += "\nCompilation successful. Choose where to save the .bin file.";
        CustomFileDialog::OpenDialog("IDE_ExportBinDlg", "Export Binary As", ".bin", ".");
    }
#endif
}

static void DrawCompilerToolbar(Control::AppState& state) {
    if (ImGui::RadioButton("C", state.ide.isCMode)) {
        state.ide.isCMode = true;
        GetTextEditor().SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Assembly", !state.ide.isCMode)) {
        state.ide.isCMode = false;
        GetTextEditor().SetLanguageDefinition(GetCC65LanguageDefinition());
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
    if (CustomFileDialog::Display("IDE_OpenFile")) {
        if (CustomFileDialog::IsOk()) {
            state.ide.currentFilePath = CustomFileDialog::GetFilePathName();
            std::ifstream fileIn(state.ide.currentFilePath);
            if (fileIn) {
                std::ostringstream stringStream;
                stringStream << fileIn.rdbuf();
                state.ide.code = stringStream.str();
                GetTextEditor().SetText(state.ide.code);

                state.ide.outputLog = "Opened " + state.ide.currentFilePath;
                
                if (state.ide.currentFilePath.ends_with(".c")) {
                    state.ide.isCMode = true;
                    GetTextEditor().SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
                } else if (state.ide.currentFilePath.ends_with(".s") || state.ide.currentFilePath.ends_with(".asm")) {
                    state.ide.isCMode = false;
                    GetTextEditor().SetLanguageDefinition(GetCC65LanguageDefinition());
                }
            } else {
                state.ide.outputLog = "Failed to open " + state.ide.currentFilePath;
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleSaveFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_SaveFile")) {
        if (CustomFileDialog::IsOk()) {
            state.ide.currentFilePath = CustomFileDialog::GetFilePathName();
            std::ofstream fileOut(state.ide.currentFilePath);
            if (fileOut) {
                fileOut << state.ide.code;
                state.ide.outputLog = "Saved " + state.ide.currentFilePath;
            } else {
                state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleExportFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_ExportBinDlg")) {
        if (CustomFileDialog::IsOk()) {
            std::string exportPath = CustomFileDialog::GetFilePathName();
            std::ofstream fileOut(exportPath, std::ios::binary);
            if (fileOut && !state.ide.exportBinary.empty()) {
                fileOut.write(std::bit_cast<const char*>(state.ide.exportBinary.data()),
                            static_cast<std::streamsize>(state.ide.exportBinary.size()));
                state.ide.outputLog += "\nExported binary to " + exportPath;
                
                if (!state.ide.exportDbgPath.empty()) {
                    std::string dbgPath = exportPath.substr(0, exportPath.find_last_of('.')) + ".dbg";
                    std::ifstream src(state.ide.exportDbgPath, std::ios::binary);
                    std::ofstream dst(dbgPath, std::ios::binary);
                    if (src && dst) {
                        dst << src.rdbuf();
                        state.ide.outputLog += "\nExported debug symbols to " + dbgPath;
                    }
                }
            } else {
                state.ide.outputLog += "\nFailed to export binary to " + exportPath;
            }
            state.ide.exportBinary.clear();
            state.ide.exportDbgPath.clear();
        } else {
            state.ide.exportBinary.clear();
            state.ide.exportDbgPath.clear();
        }
        CustomFileDialog::Close();
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

#ifdef TARGET_WASM
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
        InitEditor(state);
        
        if (GetTextEditor().GetText().empty() && !state.ide.code.empty()) {
            GetTextEditor().SetText(state.ide.code);
        }
        
        GetTextEditor().Render("##CodeEditor", ImVec2(0, availableHeight - outputHeight - 30.0F));
        
        if (GetTextEditor().IsTextChanged()) {
            state.ide.code = GetTextEditor().GetText();
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
