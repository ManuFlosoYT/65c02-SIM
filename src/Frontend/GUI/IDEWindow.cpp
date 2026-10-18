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

    TextEditor::LanguageDefinition GetMOSLanguageDefinition() {
        TextEditor::LanguageDefinition langDef = TextEditor::LanguageDefinition::CPlusPlus();
        langDef.mName = "6502 Assembly";

        static const std::array<const char*, 98> opcodes = {{
            "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk", "bvc", "bvs", "clc",
            "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx", "iny", "jmp",
            "jsr", "lda", "ldx", "ldy", "lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti",
            "rts", "sbc", "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
            "bra", "phx", "phy", "plx", "ply", "stz", "trb", "tsb", "wai", "stp",
            "bbr0", "bbr1", "bbr2", "bbr3", "bbr4", "bbr5", "bbr6", "bbr7",
            "bbs0", "bbs1", "bbs2", "bbs3", "bbs4", "bbs5", "bbs6", "bbs7",
            "rmb0", "rmb1", "rmb2", "rmb3", "rmb4", "rmb5", "rmb6", "rmb7",
            "smb0", "smb1", "smb2", "smb3", "smb4", "smb5", "smb6", "smb7"
        }};

        static const std::array<const char*, 29> directives = {{
            ".byte", ".word", ".dword", ".short", ".fill", ".space", ".text", ".data", ".bss", ".rodata",
            ".section", ".globl", ".global", ".set", ".equ", ".ascii", ".asciz", ".org", ".include", ".incbin",
            ".macro", ".endm", ".endmacro", ".res", ".segment", ".export", ".import", ".align", ".addr"
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
            GetTextEditor().SetLanguageDefinition(state.ide.isCMode ? TextEditor::LanguageDefinition::CPlusPlus() : GetMOSLanguageDefinition());
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

static bool SaveCurrentFileForCompile(Control::AppState& state) {
    state.ide.outputLog.clear();
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
static CompilerFrontend::BuildResult CompileCurrentFile(Control::AppState& state) {
    CompilerFrontend::BuildType type = state.ide.isCMode ?
        CompilerFrontend::BuildType::C : CompilerFrontend::BuildType::Assembly;
    auto result = CompilerFrontend::Compile(type, state.ide.code, state.ide.currentFilePath);
    state.ide.outputLog = result.log;
    return result;
}
#endif

static void HandleCompileAndRun(Control::AppState& state) {
    if (!SaveCurrentFileForCompile(state)) {
        return;
    }

#ifndef TARGET_WASM
    auto result = CompileCurrentFile(state);

    if (result.success && !result.binary.empty()) {
        state.emulator.Pause();
        std::string errorMsg;
        if (state.emulator.InitFromMemory(result.binary, "compiled.bin", errorMsg)) {
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
    if (!SaveCurrentFileForCompile(state)) {
        return;
    }

#ifndef TARGET_WASM
    auto result = CompileCurrentFile(state);

    if (result.success && !result.binary.empty()) {
        state.ide.exportBinary = result.binary;
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
        GetTextEditor().SetLanguageDefinition(GetMOSLanguageDefinition());
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
                    GetTextEditor().SetLanguageDefinition(GetMOSLanguageDefinition());
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
            } else {
                state.ide.outputLog += "\nFailed to export binary to " + exportPath;
            }
            state.ide.exportBinary.clear();
        } else {
            state.ide.exportBinary.clear();
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
    if (ImGui::Begin("65C02 Code Editor (LLVM-MOS)", &state.ide.open)) {
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
        ImGui::InputTextMultiline("##BuildOutput", state.ide.outputLog.data(),
            state.ide.outputLog.size() + 1,
            ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

        HandleIDEFileDialogs(state);
    }
    ImGui::End();
#endif
}

}
