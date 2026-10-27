#include "Frontend/GUI/Debugger/DisassemblerWindow.h"

#include "Frontend/GUI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <fstream>
#endif
#include <imgui.h>

#include "Hardware/CPU/Disassembler.h"
#include <cstdlib>
#include <filesystem>

namespace GUI {

static void ResolveSymbolsForLocation(const Control::AppState& state, uint16_t currentAddr,
                                      std::string& labelText, std::string& sourceText) {
    if (const auto* sym = state.rom.symbols.GetSymbolAtAddress(currentAddr)) {
        labelText = sym->name;
    }
    
    if (const auto* line = state.rom.symbols.GetLineForAddress(currentAddr)) {
        if (const auto* file = state.rom.symbols.GetFile(line->fileId)) {
            sourceText = file->name + ":" + std::to_string(line->line);
        }
    }
}

static void ResolveSymbolsForOperand(const Control::AppState& state,
                                     const Hardware::DisassembledInstruction& inst,
                                     std::string& opText) {
    if (!inst.hasOperand || inst.mode == Hardware::AddressingMode::Immediate) {
        return;
    }

    const auto* opSym = state.rom.symbols.GetSymbolAtAddress(inst.operandValue);
    if (!opSym) {
        return;
    }

    switch (inst.mode) {
        case Hardware::AddressingMode::ZeroPage:
        case Hardware::AddressingMode::Absolute:
        case Hardware::AddressingMode::Relative:
            opText = opSym->name;
            break;
        case Hardware::AddressingMode::ZeroPageX:
        case Hardware::AddressingMode::AbsoluteX:
            opText = opSym->name + ",X";
            break;
        case Hardware::AddressingMode::ZeroPageY:
        case Hardware::AddressingMode::AbsoluteY:
            opText = opSym->name + ",Y";
            break;
        case Hardware::AddressingMode::Indirect:
        case Hardware::AddressingMode::ZeroPageIndirect:
            opText = "(" + opSym->name + ")";
            break;
        case Hardware::AddressingMode::IndirectX:
            opText = "(" + opSym->name + ",X)";
            break;
        case Hardware::AddressingMode::IndirectY:
            opText = "(" + opSym->name + "),Y";
            break;
        case Hardware::AddressingMode::AbsoluteIndexedIndirect:
            opText = "(" + opSym->name + ",X)";
            break;
        case Hardware::AddressingMode::ZeroPageRelative: {
            size_t comma = opText.find(',');
            if (comma != std::string::npos) {
                opText = opText.substr(0, comma + 2) + opSym->name;
            }
            break;
        }
        default:
            break;
    }
}

static void ResolveSymbolsForInstruction(const Control::AppState& state, uint16_t currentAddr,
                                         const Hardware::DisassembledInstruction& inst,
                                         std::string& labelText, std::string& opText, std::string& sourceText) {
    if (!state.rom.symbols.IsLoaded()) {
        return;
    }
    ResolveSymbolsForLocation(state, currentAddr, labelText, sourceText);
    ResolveSymbolsForOperand(state, inst, opText);
}

void DrawDisassemblerContent(Control::AppState& state) {
#ifdef TARGET_WASM
    ImGui::BeginDisabled();
    if (ImGui::Button("Load Symbols (.dbg / .elf)")) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }
            state.rom.symbols.LoadFromFile(virtualPath);
        };
        WebFileUtils::open_browser_file_picker(".dbg,.elf");
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Debug symbols are not supported in the web build.");
    }
#else
    if (ImGui::Button("Load Symbols (.dbg / .elf)")) {
        Frontend::CustomFileDialog::OpenDialog("LoadDbgDlgKey", "Load Debug Symbols", ".dbg,.elf", ".");
    }
#endif

    static bool showHybrid = true;
    ImGui::SameLine();
    ImGui::Checkbox("Hybrid C/ASM", &showHybrid);
    
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

        int lastLineNumber = -1;
        unsigned int lastFileId = 0;

        // Dissassemble a block around PC
        for (int i = 0; i < 50; i++) {
            Hardware::DisassembledInstruction instr =
                Hardware::Disassembler::Disassemble(state.emulator.GetMem(), currentAddr);

            std::string labelText;
            std::string sourceText;
            std::string opText = instr.operands;

            ResolveSymbolsForInstruction(state, currentAddr, instr, labelText, opText, sourceText);

            if (showHybrid && state.rom.symbols.IsLoaded()) {
                if (const auto* line = state.rom.symbols.GetLineForAddress(currentAddr)) {
                    if (line->line > 0 && (line->line != static_cast<unsigned int>(lastLineNumber) || line->fileId != lastFileId)) {
                        lastLineNumber = static_cast<int>(line->line);
                        lastFileId = line->fileId;

                        std::string srcCode = state.rom.symbols.GetSourceLine(line->fileId, line->line);
                        size_t startPos = srcCode.find_first_not_of(" \t\r\n");
                        if (startPos != std::string::npos) {
                            srcCode = srcCode.substr(startPos);
                        }

                        const auto* file = state.rom.symbols.GetFile(line->fileId);
                        std::string fname = file ? std::filesystem::path(file->name).filename().string() : "";

                        ImGui::TableNextRow();
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.12F, 0.22F, 0.16F, 0.85F)));
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4F, 0.85F, 0.5F, 1.0F));
                        if (!fname.empty()) {
                            ImGui::Text("// %s:%u", fname.c_str(), line->line);
                        } else {
                            ImGui::Text("// Line %u", line->line);
                        }
                        ImGui::TableSetColumnIndex(5);
                        ImGui::TextUnformatted(srcCode.c_str());
                        ImGui::PopStyleColor();
                    }
                }
            }

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

            uint16_t prevAddr = currentAddr;
            currentAddr += instr.size;
            if (currentAddr < instr.size) {
                break;  // Overflow
            }
            if (prevAddr < progCounter && currentAddr > progCounter) {
                currentAddr = progCounter;
            }
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

}  // namespace GUI
