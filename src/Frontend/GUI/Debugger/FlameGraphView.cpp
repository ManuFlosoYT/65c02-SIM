#include "Frontend/GUI/Debugger/FlameGraphView.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace GUI {

namespace {

char gProfilerFilter[128] = "";
std::vector<std::pair<uint16_t, bool>> gZoomPath;

using Hardware::FlameGraphNode;

std::shared_ptr<FlameGraphNode> FindNodeByPath(
    const std::shared_ptr<FlameGraphNode>& root,
    const std::vector<std::pair<uint16_t, bool>>& path) {
    if (!root) {
        return nullptr;
    }
    auto current = root;
    for (const auto& [addr, isInterrupt] : path) {
        std::shared_ptr<FlameGraphNode> next = nullptr;
        for (const auto& child : current->children) {
            if (child && child->address == addr && child->isInterrupt == isInterrupt) {
                next = child;
                break;
            }
        }
        if (!next) {
            return nullptr;
        }
        current = next;
    }
    return current;
}

}  // namespace

bool FlameGraphView::MatchesFilter(const std::string& name, uint16_t addr, const char* filter) {
    if (filter == nullptr || filter[0] == '\0') {
        return true;
    }
    std::string f = filter;
    for (char& c : f) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    std::string n = name;
    for (char& c : n) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (n.find(f) != std::string::npos) {
        return true;
    }

    std::array<char, 8> hexBuf{};
    std::snprintf(hexBuf.data(), hexBuf.size(), "%04x", addr);
    if (std::string(hexBuf.data()).find(f) != std::string::npos) {
        return true;
    }
    std::snprintf(hexBuf.data(), hexBuf.size(), "$%04x", addr);
    return std::string(hexBuf.data()).find(f) != std::string::npos;
}

std::string FlameGraphView::FormatHex16(uint16_t addr) {
    std::array<char, 6> buf{};
    std::snprintf(buf.data(), buf.size(), "$%04X", addr);
    return std::string(buf.data());
}

char* FlameGraphView::GetFilter() {
    return gProfilerFilter;
}

void FlameGraphView::ClearFilter() {
    gProfilerFilter[0] = '\0';
}

void FlameGraphView::Draw(Control::AppState& state, const Hardware::SubroutineProfiler::Snapshot& snapshot) {
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
        state.emulator.GetSubroutineProfiler().Clear();
        gZoomPath.clear();
    }
    ImGui::SameLine();
    bool enabled = state.emulator.GetSubroutineProfiler().IsEnabled();
    if (ImGui::Checkbox("Capture", &enabled)) {
        state.emulator.GetSubroutineProfiler().SetEnabled(enabled);
    }
    ImGui::SameLine();
    bool hasZoom = !gZoomPath.empty();
    if (!hasZoom) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Reset Zoom")) {
        gZoomPath.clear();
    }
    if (!hasZoom) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    static bool rootAtTop = false;
    ImGui::Checkbox("Root at Top", &rootAtTop);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0F * state.render.uiScale);
    ImGui::InputTextWithHint("##ProfilerFilter", "Filter...", gProfilerFilter, sizeof(gProfilerFilter));
    if (gProfilerFilter[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            gProfilerFilter[0] = '\0';
        }
    }
    ImGui::SameLine();
    ImGui::Text("Total Cycles: %llu", static_cast<unsigned long long>(snapshot.totalCycles));

    if (snapshot.totalCycles == 0 || !snapshot.root || snapshot.root->children.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7F, 0.7F, 0.7F, 1.0F),
                           "No profiler data recorded yet. Enable 'Capture' and run code in the emulator.");
        return;
    }

    auto activeRoot = FindNodeByPath(snapshot.root, gZoomPath);
    if (!activeRoot) {
        gZoomPath.clear();
        activeRoot = snapshot.root;
    }

    uint64_t activeTotalCycles = activeRoot->totalCycles;
    if (activeTotalCycles == 0) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7F, 0.7F, 0.7F, 1.0F), "Selected subtree has 0 total cycles.");
        return;
    }

    struct RenderBox {
        std::shared_ptr<FlameGraphNode> node;
        int depth = 0;
        double startFraction = 0.0;
        double widthFraction = 1.0;
        std::vector<std::pair<uint16_t, bool>> path;
    };
    std::vector<RenderBox> boxes;
    boxes.reserve(64);

    auto traverse = [&](auto& self, const std::shared_ptr<FlameGraphNode>& node, int depth,
                        double startFrac, double widthFrac,
                        std::vector<std::pair<uint16_t, bool>>& currentPath) -> void {
        if (!node || widthFrac <= 0.0) {
            return;
        }
        boxes.push_back({node, depth, startFrac, widthFrac, currentPath});
        double runningFrac = startFrac;
        for (const auto& child : node->children) {
            if (!child || child->totalCycles == 0) {
                continue;
            }
            double childWidthFrac = static_cast<double>(child->totalCycles) / static_cast<double>(activeTotalCycles);
            currentPath.push_back({child->address, child->isInterrupt});
            self(self, child, depth + 1, runningFrac, childWidthFrac, currentPath);
            currentPath.pop_back();
            runningFrac += childWidthFrac;
        }
    };

    std::vector<std::pair<uint16_t, bool>> currentPath = gZoomPath;
    traverse(traverse, activeRoot, 0, 0.0, 1.0, currentPath);

    int maxDepth = 0;
    for (const auto& box : boxes) {
        maxDepth = std::max(maxDepth, box.depth);
    }

    const float kRowHeight = 24.0F;
    float canvasHeight = (maxDepth + 1) * kRowHeight;

    if (ImGui::BeginChild("FlameGraphCanvas", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImVec2 screenOrigin = ImGui::GetCursorScreenPos();
        float contentWidth = std::max(ImGui::GetContentRegionAvail().x, 300.0F);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::Dummy(ImVec2(contentWidth, canvasHeight));

        std::vector<std::pair<uint16_t, bool>> nextZoomPath = gZoomPath;
        bool zoomChanged = false;

        for (std::size_t i = 0; i < boxes.size(); i++) {
            const auto& box = boxes[i];
            float x0 = screenOrigin.x + static_cast<float>(box.startFraction * contentWidth);
            float x1 = screenOrigin.x + static_cast<float>((box.startFraction + box.widthFraction) * contentWidth);
            if ((x1 - x0) < 0.5F) {
                continue;
            }

            float boxX0 = x0;
            float boxX1 = std::max(x0 + 1.0F, x1 - 1.0F);
            float boxY0 = rootAtTop ? (screenOrigin.y + box.depth * kRowHeight)
                                    : (screenOrigin.y + (maxDepth - box.depth) * kRowHeight);
            float boxY1 = boxY0 + kRowHeight - 1.0F;

            uint32_t hash = 2166136261U;
            for (char c : box.node->name) {
                hash = (hash ^ static_cast<uint8_t>(c)) * 16777619U;
            }
            hash = (hash ^ box.node->address);

            float h = 0.02F + 0.12F * (static_cast<float>(hash & 0xFF) / 255.0F);
            float s = 0.70F + 0.25F * (static_cast<float>((hash >> 8) & 0xFF) / 255.0F);
            float v = 0.80F + 0.15F * (static_cast<float>((hash >> 16) & 0xFF) / 255.0F);

            ImVec4 boxColor = ImColor::HSV(h, s, v);
            bool hasPenalties = (box.node->pageCrossPenalties > 0);
            if (hasPenalties) {
                boxColor = ImVec4(std::min(boxColor.x * 1.3F, 1.0F), boxColor.y * 0.6F, boxColor.z * 0.6F, 1.0F);
            }

            bool filterActive = (gProfilerFilter[0] != '\0');
            bool matchesFilter = true;
            if (filterActive) {
                matchesFilter = MatchesFilter(box.node->name, box.node->address, gProfilerFilter);
                if (!matchesFilter) {
                    boxColor.w = 0.25F;
                }
            }

            drawList->AddRectFilled(ImVec2(boxX0, boxY0), ImVec2(boxX1, boxY1), ImColor(boxColor), 2.0F);
            drawList->AddRect(ImVec2(boxX0, boxY0), ImVec2(boxX1, boxY1), IM_COL32(0, 0, 0, 50), 2.0F);

            if (hasPenalties) {
                drawList->AddRect(ImVec2(boxX0, boxY0), ImVec2(boxX1, boxY1), IM_COL32(255, 230, 0, 255), 2.0F, 0,
                                  2.0F);
            }
            if (filterActive && matchesFilter) {
                drawList->AddRect(ImVec2(boxX0, boxY0), ImVec2(boxX1, boxY1), IM_COL32(50, 255, 255, 255), 2.0F, 0,
                                  1.5F);
            }

            ImGui::SetCursorScreenPos(ImVec2(boxX0, boxY0));
            std::string btnId = "##node_" + std::to_string(i) + "_" + std::to_string(box.node->address);
            ImGui::InvisibleButton(btnId.c_str(), ImVec2(boxX1 - boxX0, boxY1 - boxY0));

            if (ImGui::IsItemHovered()) {
                drawList->AddRect(ImVec2(boxX0, boxY0), ImVec2(boxX1, boxY1), IM_COL32(255, 255, 255, 220), 2.0F, 0,
                                  2.0F);
                if (ImGui::BeginItemTooltip()) {
                    if (box.node->isInterrupt) {
                        ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "[Interrupt Handler]");
                    }
                    ImGui::Text("%s ($%04X)", box.node->name.c_str(), box.node->address);
                    ImGui::Separator();
                    double totalPct =
                        snapshot.totalCycles > 0 ? (100.0 * box.node->totalCycles / snapshot.totalCycles) : 0.0;
                    double selfPct =
                        snapshot.totalCycles > 0 ? (100.0 * box.node->selfCycles / snapshot.totalCycles) : 0.0;
                    ImGui::Text("Total Cycles: %llu (%.2f%%)",
                                static_cast<unsigned long long>(box.node->totalCycles), totalPct);
                    ImGui::Text("Self Cycles:  %llu (%.2f%%)",
                                static_cast<unsigned long long>(box.node->selfCycles), selfPct);
                    ImGui::Text("Call Count:   %u", box.node->callCount);
                    if (box.node->pageCrossPenalties > 0) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.2F, 1.0F),
                                           "⚠️ %u page cross penalties (+%u cycles in indexed modes)",
                                           box.node->pageCrossPenalties, box.node->pageCrossPenalties);
                    }
                    ImGui::Spacing();
                    ImGui::TextDisabled("Left-click: Zoom in | Right-click: Reset Zoom");
                    ImGui::EndTooltip();
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (box.node == activeRoot) {
                    if (!nextZoomPath.empty()) {
                        nextZoomPath.pop_back();
                        zoomChanged = true;
                    }
                } else {
                    nextZoomPath = box.path;
                    zoomChanged = true;
                }
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                nextZoomPath.clear();
                zoomChanged = true;
            }

            if ((boxX1 - boxX0) > 28.0F) {
                double pct = snapshot.totalCycles > 0 ? (100.0 * box.node->totalCycles / snapshot.totalCycles) : 0.0;
                std::array<char, 16> pctBuf{};
                std::snprintf(pctBuf.data(), pctBuf.size(), "%.1f%%", pct);

                std::string label = box.node->name.empty() ? FormatHex16(box.node->address) : box.node->name;
                label += " (";
                label += pctBuf.data();
                label += ")";

                drawList->PushClipRect(ImVec2(boxX0 + 2.0F, boxY0), ImVec2(boxX1 - 2.0F, boxY1), true);
                float textY = boxY0 + (kRowHeight - ImGui::GetFontSize()) * 0.5F;
                ImU32 textCol = (filterActive && !matchesFilter) ? IM_COL32(60, 60, 60, 100)
                                                                : IM_COL32(20, 20, 20, 240);
                drawList->AddText(ImVec2(boxX0 + 4.0F, textY), textCol, label.c_str());
                drawList->PopClipRect();
            }
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            nextZoomPath.clear();
            zoomChanged = true;
        }

        if (zoomChanged) {
            gZoomPath = nextZoomPath;
        }

        ImGui::EndChild();
    }
}

}  // namespace GUI
