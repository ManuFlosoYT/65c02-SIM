#include "Frontend/GUI/Debugger/ProfilerWindow.h"
#include "Frontend/GUI/Debugger/FlameGraphView.h"
#include "Frontend/GUI/Debugger/MemoryHeatmapExporter.h"
#include "Frontend/GUI/CustomFileDialog.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Hardware/Core/SubroutineProfiler.h"

using namespace Control;
using namespace Hardware;

namespace GUI {

namespace {

void DrawProfilerTooltip(std::span<const uint32_t> counts, std::span<const uint32_t> readCounts,
                         std::span<const uint32_t> writeCounts, float imgSizeX, float imgSizeY) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 imgPos = ImGui::GetItemRectMin();

    int memX = static_cast<int>((mousePos.x - imgPos.x) / imgSizeX * 256.0F);
    int memY = static_cast<int>((mousePos.y - imgPos.y) / imgSizeY * 256.0F);
    memX = std::clamp(memX, 0, 255);
    memY = std::clamp(memY, 0, 255);

    Word addr = static_cast<Word>((memY << 8) | memX);

    if (ImGui::BeginItemTooltip()) {
        ImGui::TextUnformatted("Memory Inspector");
        ImGui::Separator();

        std::ostringstream addrBuf;
        addrBuf << "Address:  0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << addr;
        ImGui::TextUnformatted(addrBuf.str().c_str());

        std::ostringstream countBuf;
        countBuf << "Accesses: " << std::dec << counts[addr];
        ImGui::TextUnformatted(countBuf.str().c_str());

        std::ostringstream readBuf;
        readBuf << "Reads:    " << std::dec << readCounts[addr];
        ImGui::TextColored(ImVec4(0.3F, 0.6F, 1.0F, 1.0F), "%s", readBuf.str().c_str());

        std::ostringstream writeBuf;
        writeBuf << "Writes:   " << std::dec << writeCounts[addr];
        ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "%s", writeBuf.str().c_str());

        ImGui::EndTooltip();
    }
}

void DrawSubroutinesTab(AppState& state, const SubroutineProfiler::Snapshot& snapshot) {
    if (!snapshot.root || snapshot.root->children.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7F, 0.7F, 0.7F, 1.0F), "No subroutine profiling data recorded yet.");
        return;
    }

    struct SubroutineMetric {
        uint16_t address = 0;
        std::string name;
        uint32_t callCount = 0;
        uint64_t totalCycles = 0;
        uint64_t selfCycles = 0;
        uint32_t pageCrossPenalties = 0;
        bool isInterrupt = false;
    };

    std::unordered_map<uint32_t, SubroutineMetric> subMap;
    auto collect = [&](auto& self, const std::shared_ptr<FlameGraphNode>& node) -> void {
        if (!node) {
            return;
        }
        if (node->address != 0 || node->name != "Root") {
            uint32_t key = (static_cast<uint32_t>(node->address) << 1) | (node->isInterrupt ? 1U : 0U);
            auto& m = subMap[key];
            m.address = node->address;
            if (m.name.empty() || (m.name[0] == '$' && !node->name.empty() && node->name[0] != '$')) {
                m.name = node->name;
            }
            m.callCount += node->callCount;
            m.totalCycles += node->totalCycles;
            m.selfCycles += node->selfCycles;
            m.pageCrossPenalties += node->pageCrossPenalties;
            m.isInterrupt = node->isInterrupt;
        }
        for (const auto& child : node->children) {
            self(self, child);
        }
    };
    collect(collect, snapshot.root);

    std::vector<SubroutineMetric> rows;
    rows.reserve(subMap.size());
    char* filter = FlameGraphView::GetFilter();
    for (auto& [_, val] : subMap) {
        if (filter[0] != '\0' && !FlameGraphView::MatchesFilter(val.name, val.address, filter)) {
            continue;
        }
        rows.push_back(std::move(val));
    }

    ImGui::SetNextItemWidth(200.0F * state.render.uiScale);
    ImGui::InputTextWithHint("##SubFilter", "Filter subroutines...", filter, 128);
    ImGui::SameLine();
    if (filter[0] != '\0' && ImGui::SmallButton("Clear Filter")) {
        FlameGraphView::ClearFilter();
    }
    ImGui::SameLine();
    ImGui::Text("Total Subroutines: %zu", rows.size());

    if (ImGui::BeginTable("SubroutineTable", 6,
                          ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                              ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 65.0F);
        ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 75.0F);
        ImGui::TableSetupColumn("Total Cycles (incl.)", ImGuiTableColumnFlags_WidthFixed, 150.0F);
        ImGui::TableSetupColumn("Self Cycles (excl.)", ImGuiTableColumnFlags_WidthFixed, 150.0F);
        ImGui::TableSetupColumn("Page Cross Alerts", ImGuiTableColumnFlags_WidthFixed, 110.0F);
        ImGui::TableHeadersRow();

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
        if (sortSpecs != nullptr && sortSpecs->SpecsCount > 0) {
            const auto& spec = sortSpecs->Specs[0];
            bool asc = (spec.SortDirection == ImGuiSortDirection_Ascending);
            std::sort(rows.begin(), rows.end(), [spec, asc](const SubroutineMetric& a, const SubroutineMetric& b) {
                switch (spec.ColumnIndex) {
                    case 0: return asc ? a.name < b.name : a.name > b.name;
                    case 1: return asc ? a.address < b.address : a.address > b.address;
                    case 2: return asc ? a.callCount < b.callCount : a.callCount > b.callCount;
                    case 3: return asc ? a.totalCycles < b.totalCycles : a.totalCycles > b.totalCycles;
                    case 4: return asc ? a.selfCycles < b.selfCycles : a.selfCycles > b.selfCycles;
                    case 5:
                        return asc ? a.pageCrossPenalties < b.pageCrossPenalties
                                   : a.pageCrossPenalties > b.pageCrossPenalties;
                    default: return false;
                }
            });
        }

        for (const auto& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (row.isInterrupt) {
                ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "[INT] %s", row.name.c_str());
            } else {
                ImGui::TextUnformatted(row.name.c_str());
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(FlameGraphView::FormatHex16(row.address).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", row.callCount);

            ImGui::TableSetColumnIndex(3);
            double totPct = snapshot.totalCycles > 0 ? (100.0 * row.totalCycles / snapshot.totalCycles) : 0.0;
            ImGui::Text("%llu (%.1f%%)", static_cast<unsigned long long>(row.totalCycles), totPct);

            ImGui::TableSetColumnIndex(4);
            double selfPct = snapshot.totalCycles > 0 ? (100.0 * row.selfCycles / snapshot.totalCycles) : 0.0;
            ImGui::Text("%llu (%.1f%%)", static_cast<unsigned long long>(row.selfCycles), selfPct);

            ImGui::TableSetColumnIndex(5);
            if (row.pageCrossPenalties > 0) {
                ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.2F, 1.0F), "⚠️ %u (+%u)", row.pageCrossPenalties,
                                   row.pageCrossPenalties);
            } else {
                ImGui::TextDisabled("-");
            }
        }

        ImGui::EndTable();
    }
}

void DrawPageCrossAlertsTab(AppState& state, const SubroutineProfiler::Snapshot& snapshot) {
    if (snapshot.pageCrossAlerts.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4F, 0.8F, 0.4F, 1.0F), "✓ No page boundary crossings detected.");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Indexed addressing modes (e.g. LDA $10FF,X or LDA ($80),Y) take 1 extra CPU cycle whenever an indexed "
            "address calculation crosses a 256-byte page boundary. When page crossings occur during execution, they "
            "are reported here with actionable buffer alignment advice.");
        return;
    }

    uint32_t totalPenalties = 0;
    for (const auto& alert : snapshot.pageCrossAlerts) {
        totalPenalties += alert.count;
    }

    ImGui::Text("Total page cross penalties: %u extra CPU cycles consumed", totalPenalties);
    ImGui::SameLine();
    if (ImGui::Button("Clear Alerts")) {
        state.emulator.GetSubroutineProfiler().Clear();
    }

    auto alerts = snapshot.pageCrossAlerts;
    if (ImGui::BeginTable("PageCrossTable", 5,
                          ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                              ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Instruction PC",
                                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 100.0F);
        ImGui::TableSetupColumn("Subroutine", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Base -> Effective Addr", ImGuiTableColumnFlags_WidthFixed, 180.0F);
        ImGui::TableSetupColumn("Penalties", ImGuiTableColumnFlags_WidthFixed, 100.0F);
        ImGui::TableSetupColumn("Optimization Advice", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
        if (sortSpecs != nullptr && sortSpecs->SpecsCount > 0) {
            const auto& spec = sortSpecs->Specs[0];
            bool asc = (spec.SortDirection == ImGuiSortDirection_Ascending);
            std::sort(alerts.begin(), alerts.end(), [spec, asc](const PageCrossAlert& a, const PageCrossAlert& b) {
                switch (spec.ColumnIndex) {
                    case 0: return asc ? a.pc < b.pc : a.pc > b.pc;
                    case 1: return asc ? a.subroutineName < b.subroutineName : a.subroutineName > b.subroutineName;
                    case 2: return asc ? a.baseAddr < b.baseAddr : a.baseAddr > b.baseAddr;
                    case 3: return asc ? a.count < b.count : a.count > b.count;
                    default: return false;
                }
            });
        }

        for (const auto& alert : alerts) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(FlameGraphView::FormatHex16(alert.pc).c_str());

            ImGui::TableSetColumnIndex(1);
            std::string subLabel =
                alert.subroutineName.empty()
                    ? FlameGraphView::FormatHex16(alert.subroutineAddr)
                    : (alert.subroutineName + " (" + FlameGraphView::FormatHex16(alert.subroutineAddr) + ")");
            ImGui::TextUnformatted(subLabel.c_str());

            ImGui::TableSetColumnIndex(2);
            std::string addrRange = FlameGraphView::FormatHex16(alert.baseAddr) + " -> " + FlameGraphView::FormatHex16(alert.effectiveAddr);
            ImGui::TextUnformatted(addrRange.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.2F, 1.0F), "%u (+%u cyc)", alert.count, alert.count);

            ImGui::TableSetColumnIndex(4);
            std::string advice = "Align buffer at " + FlameGraphView::FormatHex16(alert.baseAddr) +
                                 " to 256 bytes (.align 256) to eliminate penalty";
            ImGui::TextUnformatted(advice.c_str());
        }

        ImGui::EndTable();
    }
}

void DrawSymbolLoader(AppState& state) {
#ifdef TARGET_WASM
    if (ImGui::Button("Load Symbols (.dbg / .elf)")) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }
            if (state.rom.symbols.LoadFromFile(virtualPath)) {
                state.emulator.GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
            }
        };
        WebFileUtils::open_browser_file_picker(".dbg,.elf");
    }
#else
    if (ImGui::Button("Load Symbols (.dbg / .elf)")) {
        Frontend::CustomFileDialog::OpenDialog("ProfilerLoadDbgKey", "Load Debug Symbols", ".dbg,.elf", ".");
    }
#endif
    ImGui::SameLine();
    if (state.rom.symbols.IsLoaded()) {
        std::string loadedText = "Symbols: " + std::to_string(state.rom.symbols.GetSymbolCount()) + " loaded";
        ImGui::TextColored(ImVec4(0.4F, 0.85F, 0.4F, 1.0F), "%s", loadedText.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.7F, 0.7F, 0.7F, 1.0F), "No symbols loaded");
    }
}

}  // namespace

void DrawProfilerWindow(AppState& state) {
    state.emulator.GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
    auto snapshot = state.emulator.GetSubroutineProfiler().GetSnapshot();

    DrawSymbolLoader(state);
    ImGui::Spacing();

    std::span<const uint32_t> counts(state.emulator.GetProfilerCounts(), 65536);
    std::span<const uint32_t> readCounts(state.emulator.GetProfilerReadCounts(), 65536);
    std::span<const uint32_t> writeCounts(state.emulator.GetProfilerWriteCounts(), 65536);
    static std::array<unsigned char, 256ULL * 256ULL * 3ULL> pixels{};
    static bool profilerDirty = true;
    static uint64_t lastProfilerCycles = 0;
    uint64_t currentCycles = state.emulator.GetTotalCycles();
    bool cyclesChanged = (currentCycles != lastProfilerCycles);

    if (ImGui::BeginTabBar("ProfilerTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Flame Graph")) {
            FlameGraphView::Draw(state, snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Subroutines")) {
            DrawSubroutinesTab(state, snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Page Cross Alerts")) {
            DrawPageCrossAlertsTab(state, snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory Heatmap")) {
            if ((!state.emulator.IsPaused() && !state.emulator.IsHalted()) || profilerDirty || cyclesChanged) {
                MemoryHeatmapExporter::UpdateTexture(state, readCounts, writeCounts, pixels);
                lastProfilerCycles = currentCycles;
                profilerDirty = false;
            }

            MemoryHeatmapExporter::DrawControls(state, readCounts, writeCounts, pixels, profilerDirty);

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float size = std::min(avail.x, avail.y) * 0.9F;
            if (size > 10.0F) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - size) * 0.5F);
                ImGui::Image((ImTextureID)(intptr_t)state.render.profilerTexture,
                             ImVec2(size, size));

                if (ImGui::IsItemHovered()) {
                    DrawProfilerTooltip(counts, readCounts, writeCounts, size, size);
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

#ifndef TARGET_WASM
    MemoryHeatmapExporter::HandleDialogs(state, readCounts, writeCounts, pixels);
#endif
}

}  // namespace GUI
