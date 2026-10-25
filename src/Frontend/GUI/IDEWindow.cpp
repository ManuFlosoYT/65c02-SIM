#include "IDEWindow.h"
#include <bit>
#include "Frontend/GUI/CustomFileDialog.h"
#include "Frontend/GUI/IDEUtils.h"
#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"
#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <format>
#include <cctype>
#include <TextEditor.h>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#include "Frontend/Compiler/CompilerVFS.h"
#endif

namespace GUI {

using Frontend::CustomFileDialog;

namespace {
    struct IDETab {
        uint32_t id = 0;
        std::string title = "Untitled";
        std::string filePath;
        std::string code;
        TextEditor editor;
        bool isReadOnly = false;
        bool requestSelect = false;
    };

    std::vector<IDETab>& GetTabs() {
        static std::vector<IDETab> tabs;
        return tabs;
    }

    size_t& GetActiveTabIndex() {
        static size_t activeIndex = 0;
        return activeIndex;
    }

    uint32_t& GetNextTabId() {
        static uint32_t nextId = 1;
        return nextId;
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

    void SetEditorText(TextEditor& editor, const std::string& text) {
        std::string sanitized = text;
        GUI::StripTrailingNewline(sanitized);
        editor.SetText(sanitized);
    }

    void InitTabs(Control::AppState& state) {
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

    void UpdateActiveTabLanguageDefinition(Control::AppState& state) {
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

    std::string ResolveLibPath(const std::string& lib) {
        static const std::array<const char*, 10> searchDirs = {
            "sdk/microdos/Libs/",
            "SDK/sdk/microdos/Libs/",
            "output/SDK/sdk/microdos/Libs/",
            "../sdk/microdos/Libs/",
            "../../sdk/microdos/Libs/",
            "sdk/src/Libs/",
            "SDK/sdk/src/Libs/",
            "output/SDK/sdk/src/Libs/",
            "../sdk/src/Libs/",
            "../../sdk/src/Libs/"
        };
        for (const auto* dir : searchDirs) {
            std::string path = std::string(dir) + lib;
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
#ifndef TARGET_WASM
        const std::string vfsDir = CompilerVFS::GetTempDir();
        if (!vfsDir.empty()) {
            std::string vfsPath = vfsDir + "/include/Libs/" + lib;
            if (std::filesystem::exists(vfsPath)) {
                return vfsPath;
            }
        }
#endif
        return {};
    }

    void OpenLibInTab(const std::string& lib) {
        auto& tabs = GetTabs();
        for (auto& tab : tabs) {
            if (tab.title == lib) {
                tab.requestSelect = true;
                return;
            }
        }

        std::string path = ResolveLibPath(lib);
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

#ifndef TARGET_WASM
    std::vector<CompilerFrontend::Diagnostic>& GetLastDiagnostics() {
        static std::vector<CompilerFrontend::Diagnostic> diagnostics;
        return diagnostics;
    }

    std::optional<size_t> FindTabForDiagnostic(const CompilerFrontend::Diagnostic& diag) {
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

    void ApplyDiagnosticsToTabs(const std::vector<CompilerFrontend::Diagnostic>& diags) {
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

    void JumpToDiagnostic(const CompilerFrontend::Diagnostic& diag) {
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
#endif

    void InsertIncludeInActiveTab(Control::AppState& state, const std::string& lib) {
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

    std::string GetSanitizedTabCode(IDETab& tab) {
        std::string code = tab.editor.GetText();
        StripSpacesFromTrailingBlankLines(code);
        tab.code = code;
        return code;
    }

    std::vector<std::string> GetAvailableLibraries(Control::IDEMode mode) {
        std::vector<std::string> libs;
        std::unordered_set<std::string> seen;

        auto scanDir = [&](const std::string& dirPath) {
            std::error_code ec;
            std::vector<std::string> files;
            if (std::filesystem::exists(dirPath, ec) && std::filesystem::is_directory(dirPath, ec)) {
                for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec)) {
                    if (entry.is_regular_file(ec) && entry.path().extension() == ".h") {
                        files.push_back(entry.path().filename().string());
                    }
                }
                std::sort(files.begin(), files.end());
            }
            return files;
        };

        if (mode == Control::IDEMode::MicroDOS) {
            const std::array<const char*, 5> microdosDirs = {
                "sdk/microdos/Libs", "SDK/sdk/microdos/Libs", "output/SDK/sdk/microdos/Libs", "../sdk/microdos/Libs", "../../sdk/microdos/Libs"
            };
            bool scannedMicrodos = false;
            for (const auto* dir : microdosDirs) {
                auto files = scanDir(dir);
                if (!files.empty()) {
                    scannedMicrodos = true;
                    for (const auto& file : files) {
                        if (seen.find(file) == seen.end()) {
                            seen.insert(file);
                            libs.push_back(file);
                        }
                    }
                }
            }

            if (!scannedMicrodos) {
                static const std::array<const char*, 3> microdosLibs = {"app-bios.h", "System.h", "tui.h"};
                for (const auto* lib : microdosLibs) {
                    if (seen.find(lib) == seen.end()) {
                        seen.insert(lib);
                        libs.emplace_back(lib);
                    }
                }
            }
        }

        const std::array<const char*, 5> standardDirs = {
            "sdk/src/Libs", "SDK/sdk/src/Libs", "output/SDK/sdk/src/Libs", "../sdk/src/Libs", "../../sdk/src/Libs"
        };
        bool scannedAny = false;
        for (const auto* dir : standardDirs) {
            auto files = scanDir(dir);
            if (!files.empty()) {
                scannedAny = true;
                for (const auto& file : files) {
                    if (seen.find(file) == seen.end()) {
                        seen.insert(file);
                        libs.push_back(file);
                    }
                }
            }
        }

        if (!scannedAny) {
            static const std::array<const char*, 8> standardLibs = {
                "BIOS.h", "GPU.h", "GPUDoubleBuffer.h", "LCD.h", "NET.h", "SD.h", "SID.h", "VIA.h"
            };
            for (const auto* lib : standardLibs) {
                if (seen.find(lib) == seen.end()) {
                    seen.insert(lib);
                    libs.emplace_back(lib);
                }
            }
        }

        return libs;
    }

    void DrawSDKLibrariesSidebar(Control::AppState& state, float height) {
        ImGui::BeginChild("##IDELibsSidebar", ImVec2(170.0F, height), true);
        ImGui::TextUnformatted("SDK Libraries");
        ImGui::Separator();

        const std::vector<std::string> libs = GetAvailableLibraries(state.ide.mode);
        auto& tabs = GetTabs();
        auto& activeIndex = GetActiveTabIndex();

        for (const auto& lib : libs) {
            bool isSelected = false;
            if (activeIndex < tabs.size() && tabs.at(activeIndex).title == lib) {
                isSelected = true;
            }

            if (ImGui::Selectable(lib.c_str(), isSelected)) {
                OpenLibInTab(lib);
            }

            if (ImGui::BeginPopupContextItem()) {
                std::string menuLabel = "Include \"" + lib + "\"";
                if (ImGui::MenuItem(menuLabel.c_str())) {
                    InsertIncludeInActiveTab(state, lib);
                }
                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
    }

    std::string GetDebugTooltip(Control::AppState& state, const std::string& word) {
        if ((state.emulator.IsRunning() && !state.emulator.IsPaused()) || word.empty()) {
            return {};
        }

        if (state.rom.symbols.IsLoaded() || state.rom.symbols.GetSymbolCount() > 0) {
            if (const auto* sym = state.rom.symbols.FindSymbol(word)) {
                if (sym->val >= 0) {
                    auto addr = static_cast<uint16_t>(sym->val);
                    const auto& bus = state.emulator.GetMem();
                    uint8_t byteVal = bus.ReadDirect(addr);
                    uint8_t highByte = bus.ReadDirect(static_cast<uint16_t>(addr + 1));
                    auto wordVal = static_cast<uint16_t>(byteVal | (highByte << 8));

                    return std::format(
                        "Symbol: {}\n"
                        "Address: 0x{:04X}\n"
                        "Hex: 0x{:02X} (Word: 0x{:04X})\n"
                        "Dec: {} (Word: {})\n"
                        "Bin: 0b{:08b}",
                        sym->name, addr, byteVal, wordVal, byteVal, wordVal, byteVal);
                }
            }
        }

        std::string lower = word;
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        const auto& cpu = state.emulator.GetCPU();
        if (lower == "a") {
            uint8_t val = cpu.A;
            return std::format(
                "Register: A\n"
                "Hex: 0x{:02X}\n"
                "Dec: {}\n"
                "Bin: 0b{:08b}",
                val, val, val);
        }
        if (lower == "x") {
            uint8_t val = cpu.X;
            return std::format(
                "Register: X\n"
                "Hex: 0x{:02X}\n"
                "Dec: {}\n"
                "Bin: 0b{:08b}",
                val, val, val);
        }
        if (lower == "y") {
            uint8_t val = cpu.Y;
            return std::format(
                "Register: Y\n"
                "Hex: 0x{:02X}\n"
                "Dec: {}\n"
                "Bin: 0b{:08b}",
                val, val, val);
        }
        if (lower == "pc") {
            uint16_t val = cpu.PC;
            return std::format(
                "Register: PC\n"
                "Hex: 0x{:04X}\n"
                "Dec: {}\n"
                "Bin: 0b{:08b} {:08b}",
                val, val, (val >> 8) & 0xFF, val & 0xFF);
        }
        if (lower == "sp") {
            uint16_t val = cpu.SP;
            return std::format(
                "Register: SP\n"
                "Hex: 0x{:04X} (0x{:02X})\n"
                "Dec: {} ({})\n"
                "Bin: 0b{:08b}",
                val, val & 0xFF, val, val & 0xFF, val & 0xFF);
        }
        if (lower == "p" || lower == "sr" || lower == "flags") {
            uint8_t val = cpu.GetStatus();
            return std::format(
                "Register: P (Flags)\n"
                "Hex: 0x{:02X}\n"
                "Dec: {}\n"
                "Bin: 0b{:08b}",
                val, val, val);
        }

        return {};
    }

    void DrawTabsAndEditor(Control::AppState& state, float width, float height) {
        ImGui::BeginChild("##EditorChild", ImVec2(width, height), false);

        auto& tabs = GetTabs();
        auto& activeIndex = GetActiveTabIndex();

        if (ImGui::BeginTabBar("##IDETabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable)) {
            std::optional<size_t> tabToClose;

            for (size_t i = 0; i < tabs.size(); ++i) {
                auto& tab = tabs.at(i);
                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (tab.requestSelect) {
                    flags |= ImGuiTabItemFlags_SetSelected;
                    tab.requestSelect = false;
                }

                bool open = true;
                bool* pOpen = &open;

                std::string tabLabel = tab.title + (tab.isReadOnly ? " (read-only)" : "") + "###Tab_" + std::to_string(tab.id);
                if (ImGui::BeginTabItem(tabLabel.c_str(), pOpen, flags)) {
                    activeIndex = i;
                    tab.editor.SetTooltipCallback([&state](const std::string& word) {
                        return GetDebugTooltip(state, word);
                    });
                    tab.editor.Render("##CodeEditor", ImVec2(0.0F, height - 35.0F));

                    if (tab.editor.IsTextChanged()) {
                        GetSanitizedTabCode(tab);
                        if (!tab.isReadOnly) {
                            state.ide.code = tab.code;
                        }
                    }

                    if (!tab.isReadOnly) {
                        state.ide.currentFilePath = tab.filePath;
                        state.ide.code = tab.code;
                    }

                    ImGui::EndTabItem();
                }

                if (!open) {
                    tabToClose = i;
                }
            }

            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
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

            ImGui::EndTabBar();

            if (tabToClose.has_value() && tabToClose.value() < tabs.size()) {
                tabs.erase(tabs.begin() + static_cast<ptrdiff_t>(tabToClose.value()));
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
        }

        if (tabs.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("No file open. Click '+' or 'Open File' to start editing.");
        }

        ImGui::EndChild();
    }
}

static void DrawFileToolbar(Control::AppState& state) {
    if (ImGui::Button("Open File")) {
        CustomFileDialog::OpenDialog("IDE_OpenFile", "Open Source File", ".c,.s,.asm,.mid,.midi,.*", ".");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Open source code or MIDI file");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    bool canSave = (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly);

    ImGui::BeginDisabled(!canSave);
    if (ImGui::Button("Save")) {
        if (state.ide.currentFilePath.empty()) {
            CustomFileDialog::OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
        } else {
            if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly) {
                state.ide.code = GetSanitizedTabCode(tabs.at(activeIndex));
            }
            std::ofstream out(state.ide.currentFilePath);
            if (out) {
                out << state.ide.code;
                state.ide.outputLog = "Saved " + state.ide.currentFilePath;
            } else {
                state.ide.outputLog = "Failed to save " + state.ide.currentFilePath;
            }
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canSave) {
            ImGui::TextUnformatted("Active file is read-only or no file is open");
        } else {
            ImGui::TextUnformatted("Save current file to disk");
        }
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As")) {
        CustomFileDialog::OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canSave) {
            ImGui::TextUnformatted("Active file is read-only or no file is open");
        } else {
            ImGui::TextUnformatted("Save current file with a new filename");
        }
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();

    if (tabs.empty()) {
        ImGui::TextUnformatted("File: (None)");
    } else if (!state.ide.currentFilePath.empty()) {
        std::string label = "File: " + state.ide.currentFilePath;
        ImGui::TextUnformatted(label.c_str());
    } else {
        ImGui::TextUnformatted("File: (Unsaved)");
    }
}

static bool SaveCurrentFileForCompile(Control::AppState& state) {
    state.ide.outputLog.clear();
    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    if (activeIndex >= tabs.size() || tabs.at(activeIndex).isReadOnly) {
        state.ide.outputLog = "Error: No active source file to compile.";
        return false;
    }
    state.ide.code = GetSanitizedTabCode(tabs.at(activeIndex));
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
static CompilerFrontend::BuildResult CompileCurrentFile(Control::AppState& state) {
    CompilerFrontend::BuildType type = CompilerFrontend::BuildType::C;
    if (state.ide.mode == Control::IDEMode::ASM) {
        type = CompilerFrontend::BuildType::Assembly;
    } else if (state.ide.mode == Control::IDEMode::MicroDOS) {
        type = CompilerFrontend::BuildType::MicroDOS;
    }
    auto result = CompilerFrontend::Compile(type, state.ide.code, state.ide.currentFilePath);
    state.ide.outputLog = result.log;
    GetLastDiagnostics() = result.diagnostics;
    ApplyDiagnosticsToTabs(result.diagnostics);
    if (!result.success && !result.diagnostics.empty()) {
        JumpToDiagnostic(result.diagnostics.front());
    }
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
        if (state.ide.mode == Control::IDEMode::MicroDOS) {
            state.ide.outputLog += "\nCompilation successful. Choose where to save the .app file.";
            CustomFileDialog::OpenDialog("IDE_ExportAppDlg", "Export MicroDOS App As", ".app", ".");
        } else {
            state.ide.outputLog += "\nCompilation successful. Choose where to save the .bin file.";
            CustomFileDialog::OpenDialog("IDE_ExportBinDlg", "Export Binary As", ".bin", ".");
        }
    }
#endif
}

static void DrawCompilerToolbar(Control::AppState& state) {
    bool isC = (state.ide.mode == Control::IDEMode::C);
    bool isAsm = (state.ide.mode == Control::IDEMode::ASM);
    bool isMicroDOS = (state.ide.mode == Control::IDEMode::MicroDOS);

    if (ImGui::RadioButton("C", isC)) {
        state.ide.mode = Control::IDEMode::C;
        state.ide.isCMode = true;
        UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ASM", isAsm)) {
        state.ide.mode = Control::IDEMode::ASM;
        state.ide.isCMode = false;
        UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MicroDOS (C)", isMicroDOS)) {
        state.ide.mode = Control::IDEMode::MicroDOS;
        state.ide.isCMode = true;
        UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MIDI", state.ide.mode == Control::IDEMode::MIDI)) {
        state.ide.mode = Control::IDEMode::MIDI;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    auto& tabs = GetTabs();
    auto& activeIndex = GetActiveTabIndex();
    bool canCompile = (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly);

    ImGui::BeginDisabled(!canCompile);
    if (ImGui::Button("Compile & Run", ImVec2(180.0F, 0.0F))) {
        HandleCompileAndRun(state);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canCompile) {
            ImGui::TextUnformatted("Active tab is read-only or empty");
        } else {
            ImGui::TextUnformatted("Compile source code and run on simulator");
        }
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    if (ImGui::Button("Compile & Export", ImVec2(180.0F, 0.0F))) {
        HandleCompileAndExport(state);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canCompile) {
            ImGui::TextUnformatted("Active tab is read-only or empty");
        } else {
            ImGui::TextUnformatted("Compile source code and export binary ROM");
        }
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();
}

static void DrawToolbar(Control::AppState& state) {
    DrawFileToolbar(state);
    ImGui::Separator();
    DrawCompilerToolbar(state);
}

static void HandleOpenFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_OpenFile")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            if (filePath.ends_with(".mid") || filePath.ends_with(".midi")) {
                GUI::OpenMIDIFileInWorkspace(state, filePath);
                state.ide.mode = Control::IDEMode::MIDI;
                CustomFileDialog::Close();
                return;
            }
            std::ifstream fileIn(filePath);
            if (fileIn) {
                std::ostringstream stringStream;
                stringStream << fileIn.rdbuf();
                std::string content = stringStream.str();

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
            } else {
                state.ide.outputLog = "Failed to open " + filePath;
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleSaveFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_SaveFile")) {
        if (CustomFileDialog::IsOk()) {
            state.ide.currentFilePath = CustomFileDialog::GetFilePathName();
            auto& tabs = GetTabs();
            auto& activeIndex = GetActiveTabIndex();
            if (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly) {
                tabs.at(activeIndex).filePath = state.ide.currentFilePath;
                tabs.at(activeIndex).title = std::filesystem::path(state.ide.currentFilePath).filename().string();
                state.ide.code = GetSanitizedTabCode(tabs.at(activeIndex));
            }
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
                for (const auto byte : state.ide.exportBinary) {
                    fileOut.put(static_cast<char>(byte));
                }
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

static void HandleExportAppFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_ExportAppDlg")) {
        if (CustomFileDialog::IsOk()) {
            std::string exportPath = CustomFileDialog::GetFilePathName();
            std::ofstream fileOut(exportPath, std::ios::binary);
            if (fileOut && !state.ide.exportBinary.empty()) {
                for (const auto byte : state.ide.exportBinary) {
                    fileOut.put(static_cast<char>(byte));
                }
                state.ide.outputLog += "\nExported app to " + exportPath;
            } else {
                state.ide.outputLog += "\nFailed to export app to " + exportPath;
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
    HandleExportAppFile(state);
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

        const float outputHeight = 150.0F;
        const float availableHeight = ImGui::GetContentRegionAvail().y;
        const float editorSectionHeight = availableHeight - outputHeight - 30.0F;

        InitTabs(state);

        const bool showSidebar = (state.ide.mode != Control::IDEMode::ASM);
        if (showSidebar) {
            DrawSDKLibrariesSidebar(state, editorSectionHeight);
            ImGui::SameLine();
            DrawTabsAndEditor(state, ImGui::GetContentRegionAvail().x, editorSectionHeight);
        } else {
            DrawTabsAndEditor(state, 0.0F, editorSectionHeight);
        }

        ImGui::TextUnformatted("Build Output:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) {
            state.ide.outputLog.clear();
            GetLastDiagnostics().clear();
            ApplyDiagnosticsToTabs({});
        }

        if (ImGui::BeginChild("##BuildOutputChild", ImVec2(-FLT_MIN, -FLT_MIN), true,
                              ImGuiWindowFlags_HorizontalScrollbar)) {
            if (state.ide.outputLog.empty()) {
                ImGui::TextDisabled("No build output.");
            } else {
                std::istringstream stream(state.ide.outputLog);
                std::string line;
                int lineIndex = 0;
                const auto& diags = GetLastDiagnostics();

                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    if (line.empty()) {
                        ImGui::Spacing();
                        continue;
                    }

                    const CompilerFrontend::Diagnostic* matchedDiag = nullptr;
                    for (const auto& diag : diags) {
                        if (diag.line > 0 && !diag.message.empty() && line.find(diag.message) != std::string::npos) {
                            matchedDiag = &diag;
                            break;
                        }
                    }

                    if (matchedDiag != nullptr) {
                        ImVec4 color = matchedDiag->isError ? ImVec4(1.0F, 0.4F, 0.4F, 1.0F)
                                                            : ImVec4(1.0F, 0.85F, 0.3F, 1.0F);
                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        std::string selectableLabel = line + "##diag_" + std::to_string(lineIndex);
                        if (ImGui::Selectable(selectableLabel.c_str(), false)) {
                            JumpToDiagnostic(*matchedDiag);
                        }
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Click to jump to %s:%d", matchedDiag->file.c_str(), matchedDiag->line);
                        }
                    } else if (line.find(": error:") != std::string::npos || line.find("Error:") != std::string::npos) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.4F, 0.4F, 1.0F));
                        ImGui::TextUnformatted(line.c_str());
                        ImGui::PopStyleColor();
                    } else if (line.find("Compilation Successful") != std::string::npos) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4F, 1.0F, 0.4F, 1.0F));
                        ImGui::TextUnformatted(line.c_str());
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextUnformatted(line.c_str());
                    }
                    lineIndex++;
                }
            }
            ImGui::EndChild();
        }

        HandleIDEFileDialogs(state);
    }
    ImGui::End();
#endif
}

}
