#include "Frontend/GUI/Debugger/MemoryScannerWindow.h"

#include <imgui.h>

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "Frontend/Control/MemoryScanner.h"

using namespace Control;

namespace GUI {

namespace {

struct ScannerState {
    ScanQuery query;
    std::uint16_t startAddr = 0x0000;
    std::uint16_t endAddr = 0xFFFF;
    bool unknown = false;
    std::vector<ScanCandidate> candidates;
    bool scanned = false;
    char text[64] = "";
};

std::string ReadText(Hardware::Bus& bus, std::uint16_t addr, std::size_t length) {
    std::string out;
    for (std::size_t i = 0; i < length; ++i) {
        auto byte = bus.ReadDirect(static_cast<std::uint16_t>(addr + i));
        out += (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.';
    }
    return out;
}

void DrawResults(Hardware::Bus& bus, const ScannerState& scan) {
    const bool isString = scan.query.type == ScanType::String;
    const int digits = scan.query.type == ScanType::U16 ? 4 : 2;
    if (!ImGui::BeginTable("ScanResults", 3,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 90.0F);
    ImGui::TableSetupColumn("Previous", ImGuiTableColumnFlags_WidthFixed, 110.0F);
    ImGui::TableSetupColumn("Current");
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(scan.candidates.size()));
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const auto& cand = scan.candidates.at(static_cast<std::size_t>(row));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(std::format("0x{:04X}", cand.address).c_str());
            ImGui::TableSetColumnIndex(1);
            if (!isString) {
                ImGui::TextUnformatted(std::format("{:0{}X}", cand.previous, digits).c_str());
            }
            ImGui::TableSetColumnIndex(2);
            if (isString) {
                ImGui::TextUnformatted(ReadText(bus, cand.address, scan.query.text.size()).c_str());
                continue;
            }
            auto current = static_cast<std::uint16_t>(bus.ReadDirect(cand.address));
            if (digits == 4) {
                current = static_cast<std::uint16_t>(current | (bus.ReadDirect(static_cast<std::uint16_t>(cand.address + 1)) << 8U));
            }
            const bool changed = current != cand.previous;
            if (changed) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.8F, 0.2F, 1.0F));
            }
            ImGui::TextUnformatted(std::format("{:0{}X}", current, digits).c_str());
            if (changed) {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::EndTable();
}

}  // namespace

void DrawMemoryScannerWindow(AppState& state) {
    Hardware::Bus& bus = state.emulator.GetMem();
    static ScannerState scan;
    auto read = [&bus](std::uint16_t addr) { return bus.ReadDirect(addr); };

    ImGui::TextUnformatted("Memory Scanner");
    ImGui::Separator();

    int type = static_cast<int>(scan.query.type);
    ImGui::SetNextItemWidth(120.0F);
    if (ImGui::Combo("Type", &type, "8-bit\0" "16-bit LE\0" "String\0")) {
        scan.query.type = static_cast<ScanType>(type);
        scan.candidates.clear();
        scan.scanned = false;
    }

    ImGui::SetNextItemWidth(90.0F);
    ImGui::InputScalar("Start", ImGuiDataType_U16, &scan.startAddr, nullptr, nullptr, "0x%04X",
                       ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0F);
    ImGui::InputScalar("End", ImGuiDataType_U16, &scan.endAddr, nullptr, nullptr, "0x%04X",
                       ImGuiInputTextFlags_CharsHexadecimal);

    const bool isString = scan.query.type == ScanType::String;
    if (isString) {
        ImGui::SetNextItemWidth(240.0F);
        if (ImGui::InputText("Text", scan.text, sizeof(scan.text))) {
            scan.query.text = scan.text;
        }
    } else {
        ImGui::SetNextItemWidth(90.0F);
        ImGui::InputScalar("Value", ImGuiDataType_U16, &scan.query.value, nullptr, nullptr,
                           scan.query.type == ScanType::U8 ? "%02X" : "%04X", ImGuiInputTextFlags_CharsHexadecimal);
        if (scan.query.type == ScanType::U8) {
            scan.query.value &= 0xFFU;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Unknown initial value", &scan.unknown);
    }

    ImGui::Spacing();
    if (ImGui::Button("First Scan")) {
        scan.candidates = FirstScan(scan.query, scan.startAddr, scan.endAddr, scan.unknown && !isString, read);
        scan.scanned = true;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!scan.scanned);
    if (ImGui::Button("Next Scan")) {
        NextScan(scan.candidates, scan.query, read);
    }
    ImGui::SameLine();
    if (isString) {
        scan.query.filter = ScanFilter::Exact;
    } else {
        int filter = static_cast<int>(scan.query.filter);
        ImGui::SetNextItemWidth(150.0F);
        if (ImGui::Combo("##Filter", &filter, "Exact value\0" "Increased\0" "Decreased\0" "Unchanged\0")) {
            scan.query.filter = static_cast<ScanFilter>(filter);
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Reset")) {
        scan.candidates.clear();
        scan.scanned = false;
    }
    ImGui::EndDisabled();

    ImGui::Text("%zu results", scan.candidates.size());
    DrawResults(bus, scan);
}

}  // namespace GUI

