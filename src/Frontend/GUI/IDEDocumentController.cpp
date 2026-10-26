#include "Frontend/GUI/IDEDocumentController.h"
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/SDKManager.h"
#include "Frontend/GUI/IDEUtils.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

namespace GUI {

std::vector<IDETab>& IDEDocumentController::GetTabs() {
    static std::vector<IDETab> tabs;
    return tabs;
}

size_t& IDEDocumentController::GetActiveTabIndex() {
    static size_t activeIndex = 0;
    return activeIndex;
}

uint32_t& IDEDocumentController::GetNextTabId() {
    static uint32_t nextId = 1;
    return nextId;
}

TextEditor::LanguageDefinition IDEDocumentController::GetMOSLanguageDefinition() {
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

void IDEDocumentController::SetEditorText(TextEditor& editor, const std::string& text) {
    std::string sanitized = text;
    GUI::StripTrailingNewline(sanitized);
    editor.SetText(sanitized);
}

void IDEDocumentController::InitTabs(Control::AppState& state) {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    auto& tabs = GetTabs();
    if (tabs.empty()) {
        IDETab defaultTab;
        defaultTab.id = GetNextTabId()++;
        if (!state.ide.currentFilePath.empty()) {
            defaultTab.filePath = state.ide.currentFilePath;
            defaultTab.title = std::filesystem::path(state.ide.currentFilePath).filename().string();
        } else {
            defaultTab.title = "Untitled";
        }
        defaultTab.code = state.ide.code;
        defaultTab.editor.SetPalette(TextEditor::GetDarkPalette());
        if (state.ide.mode == Control::IDEMode::ASM) {
            defaultTab.editor.SetLanguageDefinition(GetMOSLanguageDefinition());
        } else {
            defaultTab.editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
        }
        if (!defaultTab.code.empty()) {
            SetEditorText(defaultTab.editor, defaultTab.code);
        }
        tabs.push_back(std::move(defaultTab));
    }
}

void IDEDocumentController::UpdateActiveTabLanguageDefinition(Control::AppState& state) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly) {
        if (state.ide.mode == Control::IDEMode::ASM) {
            tabs.at(activeIndex).editor.SetLanguageDefinition(GetMOSLanguageDefinition());
        } else {
            tabs.at(activeIndex).editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
        }
    }
}

void IDEDocumentController::OpenLibInTab(const std::string& lib) {
    auto& tabs = GetTabs();
    for (auto& tab : tabs) {
        if (tab.title == lib) {
            tab.requestSelect = true;
            return;
        }
    }

    std::string path = Control::SDKManager::ResolveLibPath(lib);
    std::string content;
    if (!path.empty()) {
        std::ifstream fileIn(path);
        if (fileIn) {
            std::ostringstream stringStream;
            stringStream << fileIn.rdbuf();
            content = stringStream.str();
        }
    }

    IDETab tab;
    tab.id = GetNextTabId()++;
    tab.title = lib;
    tab.filePath = path;
    tab.code = content;
    tab.isReadOnly = true;
    tab.requestSelect = true;
    tab.editor.SetPalette(TextEditor::GetDarkPalette());
    tab.editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    tab.editor.SetReadOnly(true);
    SetEditorText(tab.editor, content);
    tabs.push_back(std::move(tab));
}

void IDEDocumentController::InsertIncludeInActiveTab(Control::AppState& state, const std::string& lib) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    IDETab* targetTab = nullptr;
    if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly) {
        targetTab = &tabs.at(activeIndex);
    } else {
        for (auto& tab : tabs) {
            if (!tab.isReadOnly) {
                targetTab = &tab;
                break;
            }
        }
    }

    if (targetTab == nullptr) {
        return;
    }

    const std::string currentText = targetTab->editor.GetText();
    const std::string includeHeaderQuote = "#include \"" + lib + "\"";
    const std::string includeHeaderAngle = "#include <" + lib + ">";

    if (currentText.find(includeHeaderQuote) != std::string::npos ||
        currentText.find(includeHeaderAngle) != std::string::npos) {
        return;
    }

    const std::string includeLine = "#include \"" + lib + "\"\n";
    auto curPos = targetTab->editor.GetCursorPosition();
    targetTab->editor.SetCursorPosition(TextEditor::Coordinates(0, 0));
    targetTab->editor.InsertText(includeLine);
    targetTab->editor.SetCursorPosition(TextEditor::Coordinates(curPos.mLine + 1, curPos.mColumn));
    targetTab->code = targetTab->editor.GetText();
    state.ide.code = targetTab->code;
}

std::string IDEDocumentController::GetSanitizedTabCode(IDETab& tab) {
    std::string code = tab.editor.GetText();
    StripSpacesFromTrailingBlankLines(code);
    tab.code = code;
    return code;
}

void IDEDocumentController::OpenDocument(Control::AppState& state, const std::string& filePath, const std::string& content) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();

    if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly &&
        tabs.at(activeIndex).filePath.empty() && tabs.at(activeIndex).code.empty()) {
        auto& tab = tabs.at(activeIndex);
        tab.filePath = filePath;
        tab.title = std::filesystem::path(filePath).filename().string();
        tab.code = content;
        SetEditorText(tab.editor, content);
    } else {
        IDETab newTab;
        newTab.id = GetNextTabId()++;
        newTab.filePath = filePath;
        newTab.title = std::filesystem::path(filePath).filename().string();
        newTab.code = content;
        newTab.editor.SetPalette(TextEditor::GetDarkPalette());
        SetEditorText(newTab.editor, content);
        newTab.requestSelect = true;
        tabs.push_back(std::move(newTab));
        activeIndex = tabs.size() - 1;
    }

    state.ide.currentFilePath = filePath;
    state.ide.code = content;
    state.ide.outputLog = "Opened " + filePath;

    if (filePath.ends_with(".c")) {
        state.ide.mode = Control::IDEMode::C;
        state.ide.isCMode = true;
        tabs.at(activeIndex).editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    } else if (filePath.ends_with(".s") || filePath.ends_with(".asm")) {
        state.ide.mode = Control::IDEMode::ASM;
        state.ide.isCMode = false;
        tabs.at(activeIndex).editor.SetLanguageDefinition(GetMOSLanguageDefinition());
    }
}

bool IDEDocumentController::SaveActiveDocument(Control::AppState& state, const std::string& filePath) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly) {
        tabs.at(activeIndex).filePath = filePath;
        tabs.at(activeIndex).title = std::filesystem::path(filePath).filename().string();
        state.ide.code = GetSanitizedTabCode(tabs.at(activeIndex));
    }
    state.ide.currentFilePath = filePath;
    std::ofstream fileOut(state.ide.currentFilePath);
    if (fileOut) {
        fileOut << state.ide.code;
        state.ide.outputLog = "Saved " + state.ide.currentFilePath;
        return true;
    }
    state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
    return false;
}

void IDEDocumentController::CreateNewTab(Control::AppState& state) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    IDETab newTab;
    newTab.id = GetNextTabId()++;
    newTab.title = "Untitled";
    newTab.editor.SetPalette(TextEditor::GetDarkPalette());
    if (state.ide.mode == Control::IDEMode::ASM) {
        newTab.editor.SetLanguageDefinition(GetMOSLanguageDefinition());
    } else {
        newTab.editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    }
    newTab.requestSelect = true;
    tabs.push_back(std::move(newTab));
    activeIndex = tabs.size() - 1;
    state.ide.currentFilePath.clear();
    state.ide.code.clear();
}

void IDEDocumentController::CloseTab(size_t index, Control::AppState& state) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    if (index >= tabs.size()) {
        return;
    }
    tabs.erase(tabs.begin() + static_cast<ptrdiff_t>(index));
    if (activeIndex >= tabs.size()) {
        activeIndex = tabs.empty() ? 0 : tabs.size() - 1;
    }
    if (tabs.empty()) {
        state.ide.code.clear();
        state.ide.currentFilePath.clear();
    } else if (!tabs.at(activeIndex).isReadOnly) {
        state.ide.code = tabs.at(activeIndex).code;
        state.ide.currentFilePath = tabs.at(activeIndex).filePath;
    } else {
        state.ide.currentFilePath.clear();
    }
}

#ifndef TARGET_WASM
std::vector<CompilerFrontend::Diagnostic>& IDEDocumentController::GetLastDiagnostics() {
    static std::vector<CompilerFrontend::Diagnostic> diagnostics;
    return diagnostics;
}

std::optional<size_t> IDEDocumentController::FindTabForDiagnostic(const CompilerFrontend::Diagnostic& diag) {
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    std::string diagFilename = std::filesystem::path(diag.file).filename().string();

    for (size_t i = 0; i < tabs.size(); ++i) {
        const auto& tab = tabs.at(i);
        if (!tab.filePath.empty()) {
            if (tab.filePath == diag.file ||
                std::filesystem::path(tab.filePath).filename().string() == diagFilename) {
                return i;
            }
        }
        if (tab.title == diagFilename) {
            return i;
        }
        if (!tab.isReadOnly && i == activeIndex) {
            if (diagFilename.find(".c") != std::string::npos || diagFilename.find(".s") != std::string::npos) {
                return i;
            }
        }
    }
    return std::nullopt;
}

void IDEDocumentController::ApplyDiagnosticsToTabs(const std::vector<CompilerFrontend::Diagnostic>& diags) {
    auto& tabs = GetTabs();
    for (auto& tab : tabs) {
        tab.editor.SetErrorMarkers({});
    }

    std::unordered_map<size_t, TextEditor::ErrorMarkers> tabMarkers;
    for (const auto& diag : diags) {
        if (diag.line <= 0) {
            continue;
        }
        auto tabIdx = FindTabForDiagnostic(diag);
        if (tabIdx.has_value() && tabIdx.value() < tabs.size()) {
            auto& markers = tabMarkers[tabIdx.value()];
            auto it = markers.find(diag.line);
            if (it != markers.end()) {
                it->second += "\n" + diag.message;
            } else {
                markers[diag.line] = diag.message;
            }
        }
    }

    for (const auto& [idx, markers] : tabMarkers) {
        tabs.at(idx).editor.SetErrorMarkers(markers);
    }
}

void IDEDocumentController::JumpToDiagnostic(const CompilerFrontend::Diagnostic& diag) {
    if (diag.line <= 0) {
        return;
    }

    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    auto tabIdx = FindTabForDiagnostic(diag);

    if (tabIdx.has_value() && tabIdx.value() < tabs.size()) {
        activeIndex = tabIdx.value();
        auto& tab = tabs.at(activeIndex);
        tab.requestSelect = true;
        int line = std::max(0, diag.line - 1);
        int col = std::max(0, diag.column - 1);
        tab.editor.SetCursorPosition(TextEditor::Coordinates(line, col));
        return;
    }

    std::string diagFilename = std::filesystem::path(diag.file).filename().string();
    if (diagFilename.ends_with(".h")) {
        OpenLibInTab(diagFilename);
        if (!tabs.empty()) {
            activeIndex = tabs.size() - 1;
            auto& tab = tabs.back();
            int line = std::max(0, diag.line - 1);
            int col = std::max(0, diag.column - 1);
            tab.editor.SetCursorPosition(TextEditor::Coordinates(line, col));
        }
    }
}

void IDEDocumentController::ClearDiagnostics() {
    GetLastDiagnostics().clear();
    ApplyDiagnosticsToTabs({});
}
#endif

} // namespace GUI
