#include "Frontend/GUI/Debugger/NetworkSnifferWindow.h"

#include <imgui.h>

#include <cctype>
#include <format>
#include <string>
#include <vector>

#include "Frontend/GUI/Style/Style.h"
#include "Hardware/Comm/ESP8266.h"

namespace GUI {

namespace {

void DrawDirectionBadge(Hardware::TrafficDirection dir) {
    ImVec4 color;
    const char* text = "";
    switch (dir) {
        case Hardware::TrafficDirection::TX:
            color = Theme::AccentPrimary;
            text = "TX";
            break;
        case Hardware::TrafficDirection::RX:
            color = Theme::AccentActive;
            text = "RX";
            break;
        case Hardware::TrafficDirection::NetTX:
            color = ImVec4(1.0F, 0.7F, 0.2F, 1.0F);
            text = "NET TX";
            break;
        case Hardware::TrafficDirection::NetRX:
            color = ImVec4(0.8F, 0.4F, 1.0F, 1.0F);
            text = "NET RX";
            break;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

const char* ChannelToString(Hardware::TrafficChannel ch) {
    switch (ch) {
        case Hardware::TrafficChannel::ATCommand:
            return "AT Cmd";
        case Hardware::TrafficChannel::ATResponse:
            return "AT Resp";
        case Hardware::TrafficChannel::DataPayload:
            return "Payload";
        case Hardware::TrafficChannel::TLS_HTTP:
            return "TLS/HTTP";
        case Hardware::TrafficChannel::System:
            return "System";
    }
    return "Unknown";
}

void DrawHexDump(const std::string& data) {
    if (data.empty()) {
        ImGui::TextDisabled("Empty payload");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    constexpr size_t kBytesPerLine = 16;
    for (size_t i = 0; i < data.size(); i += kBytesPerLine) {
        size_t count = std::min(kBytesPerLine, data.size() - i);
        std::string line = std::format("{:04X}: ", i);

        for (size_t j = 0; j < kBytesPerLine; ++j) {
            if (j < count) {
                line += std::format("{:02X} ", static_cast<unsigned char>(data[i + j]));
            } else {
                line += "   ";
            }
            if (j == 7) {
                line += " ";
            }
        }
        line += " |";
        for (size_t j = 0; j < count; ++j) {
            unsigned char c = static_cast<unsigned char>(data[i + j]);
            line += (std::isprint(c) != 0) ? static_cast<char>(c) : '.';
        }
        line += "|";
        ImGui::TextUnformatted(line.c_str());
    }
    ImGui::PopStyleColor();
}

}  // namespace

void DrawNetworkSnifferContent(Control::AppState& state) {
    auto& esp = state.emulator.GetESP8266();
    auto& snifferState = state.sniffer;

    // Top Toolbar
    ImGui::BeginGroup();
    if (ImGui::Button("Clear")) {
        esp.ClearTrafficLog();
        snifferState.selectedPacketIndex = -1;
    }
    ImGui::SameLine();

    bool isPaused = esp.IsSnifferPaused();
    if (ImGui::Button(isPaused ? "Resume Capture" : "Pause Capture")) {
        esp.SetSnifferPaused(!isPaused);
        snifferState.paused = !isPaused;
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(120.0F);
    const char* filterItems[] = {"All", "TX", "RX", "NET TX", "NET RX"};
    ImGui::Combo("Direction", &snifferState.filterDirection, filterItems, 5);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(140.0F);
    ImGui::InputTextWithHint("##Filter", "Filter text...", snifferState.searchFilter,
                             sizeof(snifferState.searchFilter));
    ImGui::SameLine();

    ImGui::Checkbox("Auto-Scroll", &snifferState.autoScroll);
    ImGui::SameLine();

    bool bridgeEnabled = state.emulator.IsACIAESPBridge();
    if (ImGui::Checkbox("Bridge ACIA <-> ESP8266", &bridgeEnabled)) {
        state.emulator.SetACIAESPBridge(bridgeEnabled);
        snifferState.aciaBridgeEnabled = bridgeEnabled;
    }
    ImGui::EndGroup();

    ImGui::Separator();

    auto packets = esp.GetTrafficLog();

    // Filter packets
    std::string searchStr = snifferState.searchFilter;
    std::vector<size_t> filteredIndices;
    filteredIndices.reserve(packets.size());

    for (size_t i = 0; i < packets.size(); ++i) {
        const auto& pkt = packets[i];
        if (snifferState.filterDirection == 1 && pkt.direction != Hardware::TrafficDirection::TX) {
            continue;
        }
        if (snifferState.filterDirection == 2 && pkt.direction != Hardware::TrafficDirection::RX) {
            continue;
        }
        if (snifferState.filterDirection == 3 && pkt.direction != Hardware::TrafficDirection::NetTX) {
            continue;
        }
        if (snifferState.filterDirection == 4 && pkt.direction != Hardware::TrafficDirection::NetRX) {
            continue;
        }

        if (!searchStr.empty()) {
            if (pkt.tag.find(searchStr) == std::string::npos && pkt.payload.find(searchStr) == std::string::npos) {
                continue;
            }
        }
        filteredIndices.push_back(i);
    }

    // Split layout: Packet Table (top 60%), Packet Inspector (bottom 40%)
    float availH = ImGui::GetContentRegionAvail().y;
    float tableH = availH * 0.58F;
    float inspectH = availH - tableH - 8.0F;

    // Packet Table
    if (ImGui::BeginChild("PacketTableRegion", ImVec2(0, tableH), ImGuiChildFlags_Borders)) {
        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("PacketsTable", 7, tableFlags)) {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 35.0F);
            ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 75.0F);
            ImGui::TableSetupColumn("Dir", ImGuiTableColumnFlags_WidthFixed, 60.0F);
            ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_WidthFixed, 65.0F);
            ImGui::TableSetupColumn("Link", ImGuiTableColumnFlags_WidthFixed, 40.0F);
            ImGui::TableSetupColumn("Summary / Tag", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 55.0F);
            ImGui::TableHeadersRow();

            for (size_t idx : filteredIndices) {
                const auto& pkt = packets[idx];
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                std::string idStr = std::format("{}", idx + 1);
                bool isSelected = (snifferState.selectedPacketIndex == static_cast<int>(idx));
                if (ImGui::Selectable(idStr.c_str(), isSelected,
                                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                    snifferState.selectedPacketIndex = static_cast<int>(idx);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(pkt.timestampMs % 1000000));

                ImGui::TableNextColumn();
                DrawDirectionBadge(pkt.direction);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ChannelToString(pkt.channel));

                ImGui::TableNextColumn();
                if (pkt.linkId >= 0) {
                    ImGui::Text("%d", pkt.linkId);
                } else {
                    ImGui::TextDisabled("-");
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(pkt.tag.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%zu B", pkt.payload.size());
            }

            if (snifferState.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0F);
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    // Packet Inspector
    if (ImGui::BeginChild("PacketInspectorRegion", ImVec2(0, inspectH), ImGuiChildFlags_Borders)) {
        if (snifferState.selectedPacketIndex >= 0 &&
            snifferState.selectedPacketIndex < static_cast<int>(packets.size())) {
            const auto& sel = packets[static_cast<size_t>(snifferState.selectedPacketIndex)];

            ImGui::Text("Packet #%d | %s | %zu bytes", snifferState.selectedPacketIndex + 1,
                        ChannelToString(sel.channel), sel.payload.size());
            ImGui::SameLine();
            DrawDirectionBadge(sel.direction);

            if (ImGui::BeginTabBar("InspectorTabs")) {
                if (ImGui::BeginTabItem("ASCII View")) {
                    ImGui::BeginChild("AsciiChild", ImVec2(0, 0), ImGuiChildFlags_None,
                                      ImGuiWindowFlags_HorizontalScrollbar);
                    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMain);
                    ImGui::TextUnformatted(sel.payload.c_str());
                    ImGui::PopStyleColor();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Hex Dump")) {
                    ImGui::BeginChild("HexChild", ImVec2(0, 0), ImGuiChildFlags_None,
                                      ImGuiWindowFlags_HorizontalScrollbar);
                    DrawHexDump(sel.payload);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        } else {
            ImGui::TextDisabled("Select a packet above to inspect its payload.");
        }
    }
    ImGui::EndChild();
}

}  // namespace GUI
