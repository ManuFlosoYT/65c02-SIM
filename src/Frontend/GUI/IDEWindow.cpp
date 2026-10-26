#include "IDEWindow.h"
#include <bit>
#include "Frontend/Control/SDKManager.h"
#include "Frontend/GUI/IDEDocumentController.h"
#include "Frontend/GUI/IDEBuildDeployer.h"
#include "Frontend/GUI/CustomFileDialog.h"
#include "Frontend/GUI/IDEUtils.h"
#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <format>
#include <cctype>
#include <TextEditor.h>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

using Frontend::CustomFileDialog;

namespace {

    void DrawSDKLibrariesSidebar(Control::AppState& state, float height) {
        ImGui::BeginChild("##IDELibsSidebar", ImVec2(170.0F * state.render.uiScale, height), true);
        ImGui::TextUnformatted("SDK Libraries");
        ImGui::Separator();

        const std::vector<std::string> libs = Control::SDKManager::GetAvailableLibraries(state.ide.mode);
        auto& tabs = IDEDocumentController::GetTabs();
        auto& activeIndex = IDEDocumentController::GetActiveTabIndex();

        for (const auto& lib : libs) {
            bool isSelected = false;
            if (activeIndex < tabs.size() && tabs.at(activeIndex).title == lib) {
                isSelected = true;
            }

            if (ImGui::Selectable(lib.c_str(), isSelected)) {
                IDEDocumentController::OpenLibInTab(lib);
            }

            if (ImGui::BeginPopupContextItem()) {
                std::string menuLabel = "Include \"" + lib + "\"";
                if (ImGui::MenuItem(menuLabel.c_str())) {
                    IDEDocumentController::InsertIncludeInActiveTab(state, lib);
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

        auto& tabs = IDEDocumentController::GetTabs();
        auto& activeIndex = IDEDocumentController::GetActiveTabIndex();

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
                        IDEDocumentController::GetSanitizedTabCode(tab);
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
                IDEDocumentController::CreateNewTab(state);
            }

            ImGui::EndTabBar();

            if (tabToClose.has_value() && tabToClose.value() < tabs.size()) {
                IDEDocumentController::CloseTab(tabToClose.value(), state);
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
    auto& tabs = IDEDocumentController::GetTabs();
    auto& activeIndex = IDEDocumentController::GetActiveTabIndex();
    bool canSave = (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly);

    ImGui::BeginDisabled(!canSave);
    if (ImGui::Button("Save")) {
        if (state.ide.currentFilePath.empty()) {
            CustomFileDialog::OpenDialog("IDE_SaveFile", "Save File As", ".c,.s,.asm,.*", ".");
        } else {
            IDEDocumentController::SaveActiveDocument(state, state.ide.currentFilePath);
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

static void DrawCompilerToolbar(Control::AppState& state) {
    bool isC = (state.ide.mode == Control::IDEMode::C);
    bool isAsm = (state.ide.mode == Control::IDEMode::ASM);
    bool isMicroDOS = (state.ide.mode == Control::IDEMode::MicroDOS);

    if (ImGui::RadioButton("C", isC)) {
        state.ide.mode = Control::IDEMode::C;
        state.ide.isCMode = true;
        IDEDocumentController::UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ASM", isAsm)) {
        state.ide.mode = Control::IDEMode::ASM;
        state.ide.isCMode = false;
        IDEDocumentController::UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MicroDOS (C)", isMicroDOS)) {
        state.ide.mode = Control::IDEMode::MicroDOS;
        state.ide.isCMode = true;
        IDEDocumentController::UpdateActiveTabLanguageDefinition(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MIDI", state.ide.mode == Control::IDEMode::MIDI)) {
        state.ide.mode = Control::IDEMode::MIDI;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    auto& tabs = IDEDocumentController::GetTabs();
    auto& activeIndex = IDEDocumentController::GetActiveTabIndex();
    bool canCompile = (activeIndex < tabs.size() && !tabs.at(activeIndex).isReadOnly);

    ImGui::BeginDisabled(!canCompile);
    if (ImGui::Button("Compile & Run")) {
#ifndef TARGET_WASM
        IDEBuildDeployer::HandleCompileAndRun(state);
#endif
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
    if (ImGui::Button("Compile & Export")) {
#ifndef TARGET_WASM
        IDEBuildDeployer::HandleCompileAndExport(state);
#endif
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
                IDEDocumentController::OpenDocument(state, filePath, content);
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
            IDEDocumentController::SaveActiveDocument(state, CustomFileDialog::GetFilePathName());
        }
        CustomFileDialog::Close();
    }
}

static void HandleExportFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_ExportBinDlg")) {
        if (CustomFileDialog::IsOk()) {
            IDEBuildDeployer::ExportBinary(state, CustomFileDialog::GetFilePathName());
        } else {
            state.ide.exportBinary.clear();
        }
        CustomFileDialog::Close();
    }
}

static void HandleExportAppFile(Control::AppState& state) {
    if (CustomFileDialog::Display("IDE_ExportAppDlg")) {
        if (CustomFileDialog::IsOk()) {
            IDEBuildDeployer::ExportApp(state, CustomFileDialog::GetFilePathName());
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
    ImGui::SetNextWindowSize(ImVec2(800.0F * state.render.uiScale, 600.0F * state.render.uiScale), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("65C02 Code Editor (LLVM-MOS)", &state.ide.open)) {
        DrawToolbar(state);
        ImGui::Separator();

        const float outputHeight = 150.0F * state.render.uiScale;
        const float availableHeight = ImGui::GetContentRegionAvail().y;
        const float editorSectionHeight = availableHeight - outputHeight - (30.0F * state.render.uiScale);

        IDEDocumentController::InitTabs(state);

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
            IDEDocumentController::ClearDiagnostics();
        }

        if (ImGui::BeginChild("##BuildOutputChild", ImVec2(-FLT_MIN, -FLT_MIN), true,
                              ImGuiWindowFlags_HorizontalScrollbar)) {
            if (state.ide.outputLog.empty()) {
                ImGui::TextDisabled("No build output.");
            } else {
                std::istringstream stream(state.ide.outputLog);
                std::string line;
                int lineIndex = 0;
                const auto& diags = IDEDocumentController::GetLastDiagnostics();

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
                            IDEDocumentController::JumpToDiagnostic(*matchedDiag);
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

} // namespace GUI
