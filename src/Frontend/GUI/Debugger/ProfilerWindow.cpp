#include "Frontend/GUI/Debugger/ProfilerWindow.h"
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

void UpdateProfilerTexture(AppState& state, std::span<const uint32_t> readCounts,
                           std::span<const uint32_t> writeCounts,
                           std::array<unsigned char, 256ULL * 256ULL * 3ULL>& pixels) {
    uint32_t maxAccess = 0;
    for (std::size_t i = 0; i < readCounts.size(); i++) {
        maxAccess = std::max({maxAccess, readCounts[i], writeCounts[i]});
    }

    if (maxAccess > 0) {
        float logMax = std::log(static_cast<float>(maxAccess) + 1.0F);
        for (std::size_t i = 0; i < readCounts.size(); i++) {
            std::size_t idx = i * 3U;
            float rIntensity = std::log(static_cast<float>(writeCounts[i]) + 1.0F) / logMax;
            float bIntensity = std::log(static_cast<float>(readCounts[i]) + 1.0F) / logMax;
            pixels.at(idx + 0) = static_cast<unsigned char>(rIntensity * 255.0F);
            pixels.at(idx + 1) = 0;
            pixels.at(idx + 2) = static_cast<unsigned char>(bIntensity * 255.0F);
        }
    } else {
        pixels.fill(0);
    }

    glBindTexture(GL_TEXTURE_2D, state.render.profilerTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

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

std::string GenerateProfilerRawData(std::span<const uint32_t> readCounts, std::span<const uint32_t> writeCounts) {
    std::ostringstream oss;
    oss << "DIR: R/W\n";
    for (std::size_t i = 0; i < readCounts.size(); i++) {
        uint32_t r = readCounts[i];
        uint32_t w = writeCounts[i];
        if (r > 0 || w > 0) {
            oss << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << i
                << ": " << std::dec << r << '/' << w << '\n';
        }
    }
    return oss.str();
}

void DrawProfilerButtons(AppState& state, std::span<const uint32_t> readCounts,
                         std::span<const uint32_t> writeCounts, std::span<unsigned char> pixels,
                         bool& profilerDirty) {
    if (ImGui::Button("Clear Data")) {
        state.emulator.ClearProfiler();
        state.emulator.GetSubroutineProfiler().Clear();
        profilerDirty = true;
    }
    ImGui::SameLine();
#ifdef TARGET_WASM
    if (ImGui::Button("Export RAW")) {
        std::string raw = GenerateProfilerRawData(readCounts, writeCounts);
        WebFileUtils::download_file("profiler_raw.txt", std::bit_cast<const uint8_t*>(raw.data()), raw.size());
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        std::string tempPath = "/tmp/profiler.bmp";
        SDL_Surface* surface = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGB24, const_cast<unsigned char*>(pixels.data()), 256 * 3);
        if (surface != nullptr) {
            SDL_SaveBMP(surface, tempPath.c_str());
            SDL_DestroySurface(surface);
            
            std::ifstream ifs(tempPath, std::ios::binary);
            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            WebFileUtils::download_file("profiler_heatmap.bmp", buffer.data(), buffer.size());
        }
    }
#else
    (void)readCounts;
    (void)writeCounts;
    (void)pixels;
    if (ImGui::Button("Export RAW")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_RAW_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                Frontend::CustomFileDialog::OpenDialog("SaveProfilerRawKey", "Save Profiler Raw Data", ".txt", ".",
                                                        filenameBuffer.data());
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export as BMP")) {
        if (!Frontend::CustomFileDialog::IsOpened()) {
            time_t rawTime = time(nullptr);
            struct tm* tm_info = localtime(&rawTime);
            std::array<char, 64> filenameBuffer{};
            if (strftime(filenameBuffer.data(), filenameBuffer.size(), "65C02-SIM_PROFILER_EXPORT_%Y-%m-%d-%H-%M-%S.",
                         tm_info) > 0) {
                Frontend::CustomFileDialog::OpenDialog("SaveProfilerImageKey", "Save Profiler Heatmap", ".bmp", ".",
                                                        filenameBuffer.data());
            }
        }
    }
#endif
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "Write (Red)");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3F, 0.6F, 1.0F, 1.0F), "Read (Blue)");
}

#ifndef TARGET_WASM
void HandleProfilerDialogs(AppState& state, std::span<const uint32_t> readCounts,
                           std::span<const uint32_t> writeCounts, std::span<unsigned char> pixels) {
    // Load Debug Symbols Dialog (.dbg / .elf)
    if (Frontend::CustomFileDialog::Display("ProfilerLoadDbgKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();
            if (state.rom.symbols.LoadFromFile(filePath)) {
                state.emulator.GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
            }
        }
        Frontend::CustomFileDialog::Close();
    }
    // Profiler Image Save Dialog
    if (Frontend::CustomFileDialog::Display("SaveProfilerImageKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string imgPath = Frontend::CustomFileDialog::GetFilePathName();

            SDL_Surface* surface = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_RGB24, pixels.data(), 256 * 3);
            if (surface != nullptr) {
                SDL_SaveBMP(surface, imgPath.c_str());
                SDL_DestroySurface(surface);
            }
        }
        Frontend::CustomFileDialog::Close();
    }

    // Profiler Raw Save Dialog
    if (Frontend::CustomFileDialog::Display("SaveProfilerRawKey")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string filePath = Frontend::CustomFileDialog::GetFilePathName();

            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                outFile << GenerateProfilerRawData(readCounts, writeCounts);
                outFile.close();
            }
        }
        Frontend::CustomFileDialog::Close();
    }
}
#endif

static char gProfilerFilter[128] = "";
static std::vector<std::pair<uint16_t, bool>> gZoomPath;

bool MatchesFilter(const std::string& name, uint16_t addr, const char* filter) {
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

std::string FormatHex16(uint16_t addr) {
    std::array<char, 6> buf{};
    std::snprintf(buf.data(), buf.size(), "$%04X", addr);
    return std::string(buf.data());
}

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

void DrawFlameGraphTab(AppState& state, const SubroutineProfiler::Snapshot& snapshot) {
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
    ImGui::SetNextItemWidth(160.0F);
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

void DrawSubroutinesTab(const SubroutineProfiler::Snapshot& snapshot) {
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
    for (auto& [_, val] : subMap) {
        if (gProfilerFilter[0] != '\0' && !MatchesFilter(val.name, val.address, gProfilerFilter)) {
            continue;
        }
        rows.push_back(std::move(val));
    }

    ImGui::SetNextItemWidth(200.0F);
    ImGui::InputTextWithHint("##SubFilter", "Filter subroutines...", gProfilerFilter, sizeof(gProfilerFilter));
    ImGui::SameLine();
    if (gProfilerFilter[0] != '\0' && ImGui::SmallButton("Clear Filter")) {
        gProfilerFilter[0] = '\0';
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
            ImGui::TextUnformatted(FormatHex16(row.address).c_str());

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
            ImGui::TextUnformatted(FormatHex16(alert.pc).c_str());

            ImGui::TableSetColumnIndex(1);
            std::string subLabel =
                alert.subroutineName.empty()
                    ? FormatHex16(alert.subroutineAddr)
                    : (alert.subroutineName + " (" + FormatHex16(alert.subroutineAddr) + ")");
            ImGui::TextUnformatted(subLabel.c_str());

            ImGui::TableSetColumnIndex(2);
            std::string addrRange = FormatHex16(alert.baseAddr) + " -> " + FormatHex16(alert.effectiveAddr);
            ImGui::TextUnformatted(addrRange.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.2F, 1.0F), "%u (+%u cyc)", alert.count, alert.count);

            ImGui::TableSetColumnIndex(4);
            std::string advice = "Align buffer at " + FormatHex16(alert.baseAddr) +
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

    if (ImGui::BeginTabBar("ProfilerTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Flame Graph")) {
            DrawFlameGraphTab(state, snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Subroutines")) {
            DrawSubroutinesTab(snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Page Cross Alerts")) {
            DrawPageCrossAlertsTab(state, snapshot);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory Heatmap")) {
            if ((!state.emulator.IsPaused() && !state.emulator.IsHalted()) || profilerDirty) {
                UpdateProfilerTexture(state, readCounts, writeCounts, pixels);
                profilerDirty = false;
            }

            DrawProfilerButtons(state, readCounts, writeCounts, pixels, profilerDirty);

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
    HandleProfilerDialogs(state, readCounts, writeCounts, pixels);
#endif
}

}  // namespace GUI
