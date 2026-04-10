#include "Frontend/GUI/Debugger/DisassemblerWindow.h"

#include "Frontend/UI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <fstream>
#endif
#include <imgui.h>

#include "Hardware/CPU/Disassembler.h"
#include <cstdlib>

namespace GUI {

static void ResolveSymbolsForLocation(const Control::AppState& state, uint16_t currentAddr,
                                      std::string& labelText, std::string& sourceText) {
    if (const auto* sym = state.rom.symbols.GetSymbolAtAddress(currentAddr)) {
        labelText = sym->name;
        if (sym->typeId >= 0) {
            if (const auto* typeInfo = state.rom.symbols.GetType(sym->typeId)) {
                (void)typeInfo;
            }
        }
    }
    
    if (const auto* line = state.rom.symbols.GetLineForAddress(currentAddr)) {
        if (const auto* file = state.rom.symbols.GetFile(line->fileId)) {
            sourceText = file->name + ":" + std::to_string(line->line);
        }
    }
}

static void ResolveDirectOperand(const Control::AppState& state, std::string& opText) {
    size_t comma = opText.find(',');
    std::string hexPart = opText.substr(1, comma != std::string::npos ? comma - 1 : std::string::npos);
    if (!hexPart.empty()) {
        char* end = nullptr;
        unsigned long val = std::strtoul(hexPart.c_str(), &end, 16);
        if (end != nullptr && *end == '\0') {
            auto opAddr = static_cast<uint16_t>(val);
            if (const auto* opSym = state.rom.symbols.GetSymbolAtAddress(opAddr)) {
                if (comma != std::string::npos) {
                    opText = opSym->name + opText.substr(comma);
                } else {
                    opText = opSym->name;
                }
            }
        }
    }
}

static void ResolveIndirectOperand(const Control::AppState& state, std::string& opText) {
    size_t endHex = opText.find_first_not_of("0123456789ABCDEFabcdef", 2);
    if (endHex != std::string::npos && endHex > 2) {
        std::string hexPart = opText.substr(2, endHex - 2);
        char* end = nullptr;
        unsigned long val = std::strtoul(hexPart.c_str(), &end, 16);
        if (end != nullptr && *end == '\0') {
            auto opAddr = static_cast<uint16_t>(val);
            if (const auto* opSym = state.rom.symbols.GetSymbolAtAddress(opAddr)) {
                opText = "(" + opSym->name + opText.substr(endHex);
            }
        }
    }
}

static void ResolveSymbolsForOperand(const Control::AppState& state, std::string& opText) {
    if (opText.empty()) {
        return;
    }
    
    if (opText[0] == '$') {
        ResolveDirectOperand(state, opText);
    } else if (opText.size() > 2 && opText[0] == '(' && opText[1] == '$') {
        ResolveIndirectOperand(state, opText);
    }
}

static void ResolveSymbolsForInstruction(const Control::AppState& state, uint16_t currentAddr,
                                         std::string& labelText, std::string& opText, std::string& sourceText) {
    if (!state.rom.symbols.IsLoaded()) {
        return;
    }
    ResolveSymbolsForLocation(state, currentAddr, labelText, sourceText);
    ResolveSymbolsForOperand(state, opText);
}

void DrawDisassemblerContent(Control::AppState& state) {
#ifdef TARGET_WASM
    ImGui::BeginDisabled();
    if (ImGui::Button("Load Symbols (.dbg)")) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }
            state.rom.symbols.LoadFromFile(virtualPath);
        };
        WebFileUtils::open_browser_file_picker(".dbg");
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Debug symbols (.dbg) are not supported in the web build.");
    }
#else
    if (ImGui::Button("Load Symbols (.dbg)")) {
        Frontend::CustomFileDialog::OpenDialog("LoadDbgDlgKey", "Load Debug Symbols", ".dbg", ".");
    }
#endif
    
    if (state.rom.symbols.IsLoaded()) {
        ImGui::SameLine();
        std::string loadedText = "Loaded " + std::to_string(state.rom.symbols.GetSymbolCount()) + " symbols and " + std::to_string(state.rom.symbols.GetLabelCount()) + " labels";
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(loadedText.c_str());
        ImGui::PopStyleColor();
    }
    
#ifndef TARGET_WASM
    if (Frontend::CustomFileDialog::Display("LoadDbgDlgKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();
            state.rom.symbols.LoadFromFile(filePath);
        }
        Frontend::CustomFileDialog::Close();
    }
#endif

    ImGui::Spacing();

    ImGui::BeginChild("DisassemblyScroll", ImVec2(0, 0), ImGuiChildFlags_Borders);

    if (ImGui::BeginTable("DisassemblyTable", 6,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter |
                              ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0F);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 60.0F);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed, 60.0F);
        ImGui::TableSetupColumn("Operands", ImGuiTableColumnFlags_WidthFixed, 100.0F);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        uint16_t progCounter = state.emulator.GetCPU().PC;
        // Search back a few bytes to find a potential start point
        // This is heuristic, but for short ranges it usually works
        uint16_t currentAddr = (progCounter > 32) ? progCounter - 32 : 0;

        // Dissassemble a block around PC
        for (int i = 0; i < 50; i++) {
            Hardware::DisassembledInstruction instr =
                Hardware::Disassembler::Disassemble(state.emulator.GetMem(), currentAddr);

            std::string labelText;
            std::string sourceText;
            std::string opText = instr.operands;

            ResolveSymbolsForInstruction(state, currentAddr, labelText, opText, sourceText);

            ImGui::TableNextRow();
            if (currentAddr == progCounter) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3F, 0.3F, 0.7F, 0.6F)));
            }

            ImGui::TableNextColumn();
            if (!labelText.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6F, 0.8F, 1.0F, 1.0F));
                ImGui::TextUnformatted(labelText.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.address.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.bytes.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.mnemonic.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(opText.c_str());

            ImGui::TableNextColumn();
            if (!sourceText.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5F, 0.5F, 0.5F, 1.0F));
                ImGui::TextUnformatted(sourceText.c_str());
                ImGui::PopStyleColor();
            }

            currentAddr += instr.size;
            if (currentAddr < instr.size) {
                break;  // Overflow
            }
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

}  // namespace GUI
