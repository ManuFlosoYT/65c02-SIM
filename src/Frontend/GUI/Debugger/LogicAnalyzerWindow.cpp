#include "Frontend/GUI/Debugger/LogicAnalyzerWindow.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "Frontend/GUI/Style/Style.h"
#include "Hardware/Core/LogicAnalyzer.h"

namespace GUI {

namespace {

struct LogicAnalyzerUIState {
    int viewMode = 2; // 0 = Waveforms, 1 = IC Pinout, 2 = Split View
    int selectedChip = 0; // 0 = W65C02S, 1 = W65C22, 2 = W65C51N
    int visibleCycles = 128;
    int scrollOffset = 0;
    bool showPortBits = false;
    bool enableCursors = false;
    int cursorA = 32;
    int cursorB = 96;
    int draggingCursor = 0; // 0 = none, 1 = A, 2 = B
    int hoveredCycleOffset = -1;
};

static LogicAnalyzerUIState uiState;

std::string ToBinary8(uint8_t val) {
    std::string s(8, '0');
    for (int i = 0; i < 8; ++i) {
        if (((val >> (7 - i)) & 1) != 0) {
            s[static_cast<size_t>(i)] = '1';
        }
    }
    return s;
}

std::string FormatTime(double seconds) {
    if (seconds < 1e-6) {
        return std::format("{:.1f} ns", seconds * 1e9);
    }
    if (seconds < 1e-3) {
        return std::format("{:.2f} µs", seconds * 1e6);
    }
    if (seconds < 1.0) {
        return std::format("{:.3f} ms", seconds * 1e3);
    }
    return std::format("{:.3f} s", seconds);
}

std::string FormatFreq(double hz) {
    if (hz >= 1e6) {
        return std::format("{:.2f} MHz", hz / 1e6);
    }
    if (hz >= 1e3) {
        return std::format("{:.2f} kHz", hz / 1e3);
    }
    return std::format("{:.1f} Hz", hz);
}

void AddTextRotated(ImDrawList* drawList, ImVec2 center, float angle, ImU32 col, const char* text) {
    int vtx_idx_1 = drawList->VtxBuffer.Size;
    ImVec2 text_size = ImGui::CalcTextSize(text);
    drawList->AddText(ImVec2(center.x - text_size.x * 0.5F, center.y - text_size.y * 0.5F), col, text);
    int vtx_idx_2 = drawList->VtxBuffer.Size;

    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);

    for (int i = vtx_idx_1; i < vtx_idx_2; i++) {
        ImDrawVert& v = drawList->VtxBuffer[i];
        float rx = v.pos.x - center.x;
        float ry = v.pos.y - center.y;
        v.pos.x = center.x + (rx * cos_a - ry * sin_a);
        v.pos.y = center.y + (rx * sin_a + ry * cos_a);
    }
}

struct PinInfo {
    int number = 0;
    const char* label = "";
    const char* desc = "";
    int state = -1; // 1 = High, 0 = Low, -1 = Inactive / NC / Power
};

void PopulatePins(int chipIdx, const Hardware::LogicSample& sample,
                  std::vector<PinInfo>& leftPins, std::vector<PinInfo>& rightPins) {
    leftPins.clear();
    rightPins.clear();

    if (chipIdx == 0) {
        // W65C02S (40 Pin PDIP)
        // Left pins (1..20)
        leftPins = {
            {1, "/VPB", "Vector Pull (Active Low output during vector fetch)", 1},
            {2, "RDY", "Ready input (halts CPU for wait states when low)", sample.rdy ? 1 : 0},
            {3, "PHI1O", "Phase 1 Clock Output", 0},
            {4, "/IRQB", "Interrupt Request (Active Low input)", sample.irq ? 0 : 1},
            {5, "/MLB", "Memory Lock (Active Low output during read-modify-write)", 1},
            {6, "/NMIB", "Non-Maskable Interrupt (Active Low input)", sample.nmi ? 0 : 1},
            {7, "SYNC", "Opcode Fetch Synchronize (High during opcode fetch)", sample.sync ? 1 : 0},
            {8, "VDD", "Positive Supply Voltage (+5V)", -1},
            {9, "A0", "Address Bus Bit 0", (sample.address >> 0) & 1},
            {10, "A1", "Address Bus Bit 1", (sample.address >> 1) & 1},
            {11, "A2", "Address Bus Bit 2", (sample.address >> 2) & 1},
            {12, "A3", "Address Bus Bit 3", (sample.address >> 3) & 1},
            {13, "A4", "Address Bus Bit 4", (sample.address >> 4) & 1},
            {14, "A5", "Address Bus Bit 5", (sample.address >> 5) & 1},
            {15, "A6", "Address Bus Bit 6", (sample.address >> 6) & 1},
            {16, "A7", "Address Bus Bit 7", (sample.address >> 7) & 1},
            {17, "A8", "Address Bus Bit 8", (sample.address >> 8) & 1},
            {18, "A9", "Address Bus Bit 9", (sample.address >> 9) & 1},
            {19, "A10", "Address Bus Bit 10", (sample.address >> 10) & 1},
            {20, "A11", "Address Bus Bit 11", (sample.address >> 11) & 1}
        };

        // Right pins (40..21)
        rightPins = {
            {40, "/RESB", "Reset Input (Active Low)", 1},
            {39, "PHI2O", "Phase 2 Clock Output", 1},
            {38, "/SOB", "Set Overflow Input (Active Low)", 1},
            {37, "PHI2", "Phase 2 Clock Input", 1},
            {36, "BE", "Bus Enable (High enables Address and Data buses)", 1},
            {35, "NC", "No Connection", -1},
            {34, "/RWB", "Read / Write Output (High = Read, Low = Write)", sample.rw ? 1 : 0},
            {33, "D0", "Data Bus Bit 0", (sample.data >> 0) & 1},
            {32, "D1", "Data Bus Bit 1", (sample.data >> 1) & 1},
            {31, "D2", "Data Bus Bit 2", (sample.data >> 2) & 1},
            {30, "D3", "Data Bus Bit 3", (sample.data >> 3) & 1},
            {29, "D4", "Data Bus Bit 4", (sample.data >> 4) & 1},
            {28, "D5", "Data Bus Bit 5", (sample.data >> 5) & 1},
            {27, "D6", "Data Bus Bit 6", (sample.data >> 6) & 1},
            {26, "D7", "Data Bus Bit 7", (sample.data >> 7) & 1},
            {25, "A15", "Address Bus Bit 15", (sample.address >> 15) & 1},
            {24, "A14", "Address Bus Bit 14", (sample.address >> 14) & 1},
            {23, "A13", "Address Bus Bit 13", (sample.address >> 13) & 1},
            {22, "A12", "Address Bus Bit 12", (sample.address >> 12) & 1},
            {21, "VSS", "Ground Reference (0V)", -1}
        };
    } else if (chipIdx == 1) {
        // W65C22 (40 Pin PDIP)
        // Left pins (1..20)
        leftPins = {
            {1, "VSS", "Ground Reference (0V)", -1},
            {2, "PA0", "Port A Bidirectional I/O Bit 0", (sample.portA >> 0) & 1},
            {3, "PA1", "Port A Bidirectional I/O Bit 1", (sample.portA >> 1) & 1},
            {4, "PA2", "Port A Bidirectional I/O Bit 2", (sample.portA >> 2) & 1},
            {5, "PA3", "Port A Bidirectional I/O Bit 3", (sample.portA >> 3) & 1},
            {6, "PA4", "Port A Bidirectional I/O Bit 4", (sample.portA >> 4) & 1},
            {7, "PA5", "Port A Bidirectional I/O Bit 5", (sample.portA >> 5) & 1},
            {8, "PA6", "Port A Bidirectional I/O Bit 6", (sample.portA >> 6) & 1},
            {9, "PA7", "Port A Bidirectional I/O Bit 7", (sample.portA >> 7) & 1},
            {10, "PB0", "Port B Bidirectional I/O Bit 0", (sample.portB >> 0) & 1},
            {11, "PB1", "Port B Bidirectional I/O Bit 1", (sample.portB >> 1) & 1},
            {12, "PB2", "Port B Bidirectional I/O Bit 2", (sample.portB >> 2) & 1},
            {13, "PB3", "Port B Bidirectional I/O Bit 3", (sample.portB >> 3) & 1},
            {14, "PB4", "Port B Bidirectional I/O Bit 4", (sample.portB >> 4) & 1},
            {15, "PB5", "Port B Bidirectional I/O Bit 5", (sample.portB >> 5) & 1},
            {16, "PB6", "Port B Bidirectional I/O Bit 6", (sample.portB >> 6) & 1},
            {17, "PB7", "Port B Bidirectional I/O Bit 7", (sample.portB >> 7) & 1},
            {18, "CB1", "Control Line B1 Input", sample.cb1 ? 1 : 0},
            {19, "CB2", "Control Line B2 Input/Output", sample.cb2 ? 1 : 0},
            {20, "VDD", "Positive Supply Voltage (+5V)", -1}
        };

        bool viaSelected = ((sample.address & 0xFFF0) == 0xD000);
        // Right pins (40..21)
        rightPins = {
            {40, "CA1", "Control Line A1 Input", sample.ca1 ? 1 : 0},
            {39, "CA2", "Control Line A2 Input/Output", sample.ca2 ? 1 : 0},
            {38, "RS0", "Register Select Bit 0", (sample.address >> 0) & 1},
            {37, "RS1", "Register Select Bit 1", (sample.address >> 1) & 1},
            {36, "RS2", "Register Select Bit 2", (sample.address >> 2) & 1},
            {35, "RS3", "Register Select Bit 3", (sample.address >> 3) & 1},
            {34, "/RESB", "Reset Input (Active Low)", 1},
            {33, "D0", "Data Bus Bit 0", (sample.data >> 0) & 1},
            {32, "D1", "Data Bus Bit 1", (sample.data >> 1) & 1},
            {31, "D2", "Data Bus Bit 2", (sample.data >> 2) & 1},
            {30, "D3", "Data Bus Bit 3", (sample.data >> 3) & 1},
            {29, "D4", "Data Bus Bit 4", (sample.data >> 4) & 1},
            {28, "D5", "Data Bus Bit 5", (sample.data >> 5) & 1},
            {27, "D6", "Data Bus Bit 6", (sample.data >> 6) & 1},
            {26, "D7", "Data Bus Bit 7", (sample.data >> 7) & 1},
            {25, "PHI2", "Phase 2 Clock Input", 1},
            {24, "CS1", "Chip Select 1 (Active High)", viaSelected ? 1 : 0},
            {23, "/CS2B", "Chip Select 2 (Active Low)", viaSelected ? 0 : 1},
            {22, "/RWB", "Read / Write Input", sample.rw ? 1 : 0},
            {21, "/IRQB", "Interrupt Request Output (Active Low)", sample.irq ? 0 : 1}
        };
    } else {
        // W65C51N (28 Pin PDIP)
        bool aciaSelected = ((sample.address & 0xFFFC) == 0xD800);
        // Left pins (1..14)
        leftPins = {
            {1, "VSS", "Ground Reference (0V)", -1},
            {2, "CS0", "Chip Select 0 (Active High)", aciaSelected ? 1 : 0},
            {3, "/CS1B", "Chip Select 1 (Active Low)", aciaSelected ? 0 : 1},
            {4, "/RESB", "Reset Input (Active Low)", 1},
            {5, "RxC", "Receive Baud Clock Input/Output", 0},
            {6, "XTLI", "Crystal Oscillator Input (1.8432 MHz)", 0},
            {7, "XTLO", "Crystal Oscillator Output", 0},
            {8, "/RTSB", "Request To Send Output (Active Low)", 1},
            {9, "/CTSB", "Clear To Send Input (Active Low)", 1},
            {10, "TxD", "Transmit Serial Data Output", sample.uartTx ? 1 : 0},
            {11, "/DTRB", "Data Terminal Ready Output (Active Low)", 1},
            {12, "RxD", "Receive Serial Data Input", sample.uartRx ? 1 : 0},
            {13, "RS0", "Register Select Bit 0", (sample.address >> 0) & 1},
            {14, "RS1", "Register Select Bit 1", (sample.address >> 1) & 1}
        };

        // Right pins (28..15)
        rightPins = {
            {28, "/RWB", "Read / Write Input (High = Read, Low = Write)", sample.rw ? 1 : 0},
            {27, "PHI2", "Phase 2 Clock Input", 1},
            {26, "/IRQB", "Interrupt Request Output (Active Low)", sample.irq ? 0 : 1},
            {25, "D7", "Data Bus Bit 7", (sample.data >> 7) & 1},
            {24, "D6", "Data Bus Bit 6", (sample.data >> 6) & 1},
            {23, "D5", "Data Bus Bit 5", (sample.data >> 5) & 1},
            {22, "D4", "Data Bus Bit 4", (sample.data >> 4) & 1},
            {21, "D3", "Data Bus Bit 3", (sample.data >> 3) & 1},
            {20, "D2", "Data Bus Bit 2", (sample.data >> 2) & 1},
            {19, "D1", "Data Bus Bit 1", (sample.data >> 1) & 1},
            {18, "D0", "Data Bus Bit 0", (sample.data >> 0) & 1},
            {17, "/DSRB", "Data Set Ready Input (Active Low)", 1},
            {16, "/DCDB", "Data Carrier Detect Input (Active Low)", 1},
            {15, "VDD", "Positive Supply Voltage (+5V)", -1}
        };
    }
}

void DrawTopBar(Control::AppState& state, Hardware::LogicAnalyzer& analyzer) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

    // --- Row 1: Controls ---
    bool isFrozen = analyzer.IsFrozen();
    if (isFrozen) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15F, 0.60F, 0.25F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20F, 0.75F, 0.32F, 1.0F));
        if (ImGui::Button("Resume##Analyzer")) {
            analyzer.SetFrozen(false);
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70F, 0.20F, 0.20F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85F, 0.25F, 0.25F, 1.0F));
        if (ImGui::Button("Freeze##Analyzer")) {
            analyzer.SetFrozen(true);
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear##Analyzer")) {
        analyzer.Clear();
        uiState.scrollOffset = 0;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // View mode selector
    const char* viewModes[] = {"Waveforms", "IC Pinout", "Split View"};
    ImGui::TextUnformatted("View:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0F);
    ImGui::Combo("##ViewMode", &uiState.viewMode, viewModes, IM_ARRAYSIZE(viewModes));

    ImGui::SameLine();
    ImGui::TextUnformatted("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0F);
    ImGui::SliderInt("##Zoom", &uiState.visibleCycles, 32, 1024, "%d cyc");

    size_t count = analyzer.GetCount();
    if (count > static_cast<size_t>(uiState.visibleCycles)) {
        ImGui::SameLine();
        int maxOffset = static_cast<int>(count - static_cast<size_t>(uiState.visibleCycles));
        ImGui::TextUnformatted("Scroll:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(85.0F);
        ImGui::SliderInt("##Scroll", &uiState.scrollOffset, 0, maxOffset, "%d");
        if (uiState.scrollOffset > 0) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Live")) {
                uiState.scrollOffset = 0;
            }
        }
    } else {
        uiState.scrollOffset = 0;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Bit-Level Ports", &uiState.showPortBits);

    // --- Row 2: Status & Measurement Cursors ---
    int targetIPS = state.emulator.GetTargetIPS();
    if (targetIPS <= 0) {
        targetIPS = 1000000;
    }
    double cycleSec = 1.0 / static_cast<double>(targetIPS);
    ImGui::TextDisabled("Clock: %.2f MHz (%s/cyc)", static_cast<double>(targetIPS) / 1e6, FormatTime(cycleSec).c_str());

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Checkbox("Cursors", &uiState.enableCursors);

    if (uiState.enableCursors && count > 0) {
        ImGui::SameLine();
        int numVis = std::min(uiState.visibleCycles, static_cast<int>(count));
        int cA = std::clamp(uiState.cursorA, 0, numVis - 1);
        int cB = std::clamp(uiState.cursorB, 0, numVis - 1);

        size_t revA = static_cast<size_t>(uiState.scrollOffset + (numVis - 1 - cA));
        size_t revB = static_cast<size_t>(uiState.scrollOffset + (numVis - 1 - cB));
        auto sA = analyzer.GetSample(revA);
        auto sB = analyzer.GetSample(revB);

        int64_t deltaCyc = std::abs(static_cast<int64_t>(sB.cycle) - static_cast<int64_t>(sA.cycle));
        double deltaSec = static_cast<double>(deltaCyc) * cycleSec;
        double freq = deltaSec > 0.0 ? (1.0 / deltaSec) : 0.0;

        ImGui::TextColored(ImVec4(0.98F, 0.80F, 0.08F, 1.0F), "A:#%llu", static_cast<unsigned long long>(sA.cycle));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.22F, 0.74F, 0.97F, 1.0F), "B:#%llu", static_cast<unsigned long long>(sB.cycle));
        ImGui::SameLine();
        ImGui::Text("Δ: %lld cyc (%s, %s)",
                    static_cast<long long>(deltaCyc),
                    FormatTime(deltaSec).c_str(),
                    FormatFreq(freq).c_str());
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();
}

void DrawICPinout(const Hardware::LogicSample& sample, float availWidth, float availHeight) {
    const char* chips[] = {"W65C02S (CPU)", "W65C22 (VIA)", "W65C51N (ACIA)"};
    ImGui::TextUnformatted("Chip:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0F);
    ImGui::Combo("##SelectIC", &uiState.selectedChip, chips, IM_ARRAYSIZE(chips));
    ImGui::SameLine();
    ImGui::TextDisabled("Cycle: #%llu", static_cast<unsigned long long>(sample.cycle));

    std::vector<PinInfo> leftPins;
    std::vector<PinInfo> rightPins;
    PopulatePins(uiState.selectedChip, sample, leftPins, rightPins);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    float canvasWidth = std::max(availWidth, 260.0F);

    const size_t pinCount = leftPins.size();
    constexpr float pinPitch = 21.0F;
    constexpr float stubLen = 14.0F;
    constexpr float stubThick = 6.0F;
    constexpr float ledRadius = 4.0F;
    constexpr float chipWidth = 100.0F;
    float chipHeight = static_cast<float>(pinCount) * pinPitch + 26.0F;

    float centerX = canvasPos.x + canvasWidth * 0.5F;
    float chipX = centerX - chipWidth * 0.5F;
    float chipY = canvasPos.y + 12.0F;

    // Background body
    drawList->AddRectFilled(ImVec2(chipX, chipY),
                            ImVec2(chipX + chipWidth, chipY + chipHeight),
                            IM_COL32(26, 29, 36, 255), 6.0F);
    drawList->AddRect(ImVec2(chipX, chipY),
                      ImVec2(chipX + chipWidth, chipY + chipHeight),
                      IM_COL32(65, 72, 88, 255), 6.0F, 0, 1.5F);

    // Notch at top
    drawList->AddCircleFilled(ImVec2(centerX, chipY), 7.5F, IM_COL32(18, 20, 25, 255));
    drawList->AddCircle(ImVec2(centerX, chipY), 7.5F, IM_COL32(65, 72, 88, 255), 16, 1.2F);

    // Orientation dot
    drawList->AddCircleFilled(ImVec2(chipX + 11.0F, chipY + 14.0F), 3.0F, IM_COL32(45, 50, 62, 255));

    // Vertical Center Chip Label (rotated 90 degrees along vertical axis)
    const char* chipTitle = (uiState.selectedChip == 0) ? "W65C02S" : (uiState.selectedChip == 1 ? "W65C22" : "W65C51N");
    float textCenterY = chipY + chipHeight * 0.5F;
    AddTextRotated(drawList, ImVec2(centerX, textCenterY), 1.5707963F, IM_COL32(220, 225, 235, 255), chipTitle);

    ImVec2 mousePos = ImGui::GetMousePos();

    // Render Left Pins (1..N)
    for (size_t i = 0; i < leftPins.size(); i++) {
        const auto& pin = leftPins[i];
        float py = chipY + 22.0F + static_cast<float>(i) * pinPitch;

        // Metal pin stub
        drawList->AddRectFilled(ImVec2(chipX - stubLen, py - stubThick * 0.5F),
                                ImVec2(chipX, py + stubThick * 0.5F),
                                IM_COL32(175, 182, 195, 255), 1.0F);

        // Pin number
        std::string numStr = std::to_string(pin.number);
        drawList->AddText(ImVec2(chipX + 4.0F, py - 7.0F), IM_COL32(130, 140, 160, 255), numStr.c_str());

        // Pin label
        ImVec2 lblSz = ImGui::CalcTextSize(pin.label);
        float lblX = chipX - stubLen - 6.0F - lblSz.x;
        drawList->AddText(ImVec2(lblX, py - 7.0F), IM_COL32(230, 235, 245, 255), pin.label);

        // LED dot
        float ledX = lblX - 8.0F - ledRadius;
        ImU32 ledCol = (pin.state == 1) ? IM_COL32(50, 220, 50, 255)
                                        : (pin.state == 0 ? IM_COL32(70, 70, 70, 255) : IM_COL32(120, 120, 120, 255));
        drawList->AddCircleFilled(ImVec2(ledX, py), ledRadius, ledCol);
        drawList->AddCircle(ImVec2(ledX, py), ledRadius, IM_COL32(20, 20, 20, 255), 12, 1.0F);

        // Hover inspection
        ImVec2 hoverMin(ledX - 5.0F, py - pinPitch * 0.5F);
        ImVec2 hoverMax(chipX + 16.0F, py + pinPitch * 0.5F);
        if (mousePos.x >= hoverMin.x && mousePos.x <= hoverMax.x &&
            mousePos.y >= hoverMin.y && mousePos.y <= hoverMax.y) {
            drawList->AddRectFilled(hoverMin, hoverMax, IM_COL32(255, 255, 255, 24), 3.0F);
            if (ImGui::BeginTooltip()) {
                ImGui::TextColored(ImVec4(0.22F, 0.74F, 0.97F, 1.0F), "Pin %d: %s", pin.number, pin.label);
                ImGui::Separator();
                ImGui::TextUnformatted(pin.desc);
                if (pin.state == 1) {
                    ImGui::TextColored(ImVec4(0.20F, 0.86F, 0.20F, 1.0F), "Logic Level: HIGH (1)");
                } else if (pin.state == 0) {
                    ImGui::TextColored(ImVec4(0.85F, 0.30F, 0.30F, 1.0F), "Logic Level: LOW (0)");
                } else {
                    ImGui::TextColored(ImVec4(0.60F, 0.60F, 0.60F, 1.0F), "Logic Level: INACTIVE / POWER");
                }
                ImGui::EndTooltip();
            }
        }
    }

    // Render Right Pins
    for (size_t i = 0; i < rightPins.size(); i++) {
        const auto& pin = rightPins[i];
        float py = chipY + 22.0F + static_cast<float>(i) * pinPitch;

        // Metal pin stub
        drawList->AddRectFilled(ImVec2(chipX + chipWidth, py - stubThick * 0.5F),
                                ImVec2(chipX + chipWidth + stubLen, py + stubThick * 0.5F),
                                IM_COL32(175, 182, 195, 255), 1.0F);

        // Pin number
        std::string numStr = std::to_string(pin.number);
        ImVec2 numSz = ImGui::CalcTextSize(numStr.c_str());
        drawList->AddText(ImVec2(chipX + chipWidth - numSz.x - 4.0F, py - 7.0F), IM_COL32(130, 140, 160, 255), numStr.c_str());

        // Pin label
        float lblX = chipX + chipWidth + stubLen + 6.0F;
        drawList->AddText(ImVec2(lblX, py - 7.0F), IM_COL32(230, 235, 245, 255), pin.label);
        ImVec2 lblSz = ImGui::CalcTextSize(pin.label);

        // LED dot
        float ledX = lblX + lblSz.x + 8.0F + ledRadius;
        ImU32 ledCol = (pin.state == 1) ? IM_COL32(50, 220, 50, 255)
                                        : (pin.state == 0 ? IM_COL32(70, 70, 70, 255) : IM_COL32(120, 120, 120, 255));
        drawList->AddCircleFilled(ImVec2(ledX, py), ledRadius, ledCol);
        drawList->AddCircle(ImVec2(ledX, py), ledRadius, IM_COL32(20, 20, 20, 255), 12, 1.0F);

        // Hover inspection
        ImVec2 hoverMin(chipX + chipWidth - 16.0F, py - pinPitch * 0.5F);
        ImVec2 hoverMax(ledX + 6.0F, py + pinPitch * 0.5F);
        if (mousePos.x >= hoverMin.x && mousePos.x <= hoverMax.x &&
            mousePos.y >= hoverMin.y && mousePos.y <= hoverMax.y) {
            drawList->AddRectFilled(hoverMin, hoverMax, IM_COL32(255, 255, 255, 24), 3.0F);
            if (ImGui::BeginTooltip()) {
                ImGui::TextColored(ImVec4(0.22F, 0.74F, 0.97F, 1.0F), "Pin %d: %s", pin.number, pin.label);
                ImGui::Separator();
                ImGui::TextUnformatted(pin.desc);
                if (pin.state == 1) {
                    ImGui::TextColored(ImVec4(0.20F, 0.86F, 0.20F, 1.0F), "Logic Level: HIGH (1)");
                } else if (pin.state == 0) {
                    ImGui::TextColored(ImVec4(0.85F, 0.30F, 0.30F, 1.0F), "Logic Level: LOW (0)");
                } else {
                    ImGui::TextColored(ImVec4(0.60F, 0.60F, 0.60F, 1.0F), "Logic Level: INACTIVE / POWER");
                }
                ImGui::EndTooltip();
            }
        }
    }

    ImGui::Dummy(ImVec2(canvasWidth, chipHeight + 24.0F));
}

enum class ChannelFormat {
    DigitalBit,
    Hex8,
    Hex16
};

struct ChannelDefinition {
    const char* name;
    ChannelFormat format;
    ImU32 color;
    std::function<uint32_t(const Hardware::LogicSample&)> getValue;
};

void DrawWaveforms(Hardware::LogicAnalyzer& analyzer, float availWidth, float availHeight) {
    size_t count = analyzer.GetCount();
    if (count == 0) {
        ImGui::TextDisabled("No logic samples recorded. Start or step emulation to capture bus activity.");
        return;
    }

    ImGui::Checkbox("Expand VIA Port Bits", &uiState.showPortBits);

    std::vector<ChannelDefinition> channels = {
        {"ADDR", ChannelFormat::Hex16, IM_COL32(56, 189, 248, 255), [](const auto& s) { return s.address; }},
        {"DATA", ChannelFormat::Hex8, IM_COL32(251, 191, 36, 255), [](const auto& s) { return s.data; }},
        {"R/W", ChannelFormat::DigitalBit, IM_COL32(52, 211, 153, 255), [](const auto& s) { return s.rw; }},
        {"SYNC", ChannelFormat::DigitalBit, IM_COL32(96, 165, 250, 255), [](const auto& s) { return s.sync; }},
        {"RDY", ChannelFormat::DigitalBit, IM_COL32(74, 222, 128, 255), [](const auto& s) { return s.rdy; }},
        {"IRQ", ChannelFormat::DigitalBit, IM_COL32(248, 113, 113, 255), [](const auto& s) { return s.irq; }},
        {"NMI", ChannelFormat::DigitalBit, IM_COL32(251, 146, 60, 255), [](const auto& s) { return s.nmi; }},
        {"PORT A", ChannelFormat::Hex8, IM_COL32(167, 139, 250, 255), [](const auto& s) { return s.portA; }}
    };

    if (uiState.showPortBits) {
        for (int b = 0; b < 8; ++b) {
            static const char* paNames[] = {"PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7"};
            channels.push_back({paNames[b], ChannelFormat::DigitalBit, IM_COL32(167, 139, 250, 220),
                                [b](const auto& s) { return (s.portA >> b) & 1; }});
        }
    }

    channels.push_back({"PORT B", ChannelFormat::Hex8, IM_COL32(192, 132, 252, 255), [](const auto& s) { return s.portB; }});

    if (uiState.showPortBits) {
        for (int b = 0; b < 8; ++b) {
            static const char* pbNames[] = {"PB0", "PB1", "PB2", "PB3", "PB4", "PB5", "PB6", "PB7"};
            channels.push_back({pbNames[b], ChannelFormat::DigitalBit, IM_COL32(192, 132, 252, 220),
                                [b](const auto& s) { return (s.portB >> b) & 1; }});
        }
    }

    channels.push_back({"CA1", ChannelFormat::DigitalBit, IM_COL32(45, 212, 191, 255), [](const auto& s) { return s.ca1; }});
    channels.push_back({"CA2", ChannelFormat::DigitalBit, IM_COL32(45, 212, 191, 255), [](const auto& s) { return s.ca2; }});
    channels.push_back({"CB1", ChannelFormat::DigitalBit, IM_COL32(129, 140, 248, 255), [](const auto& s) { return s.cb1; }});
    channels.push_back({"CB2", ChannelFormat::DigitalBit, IM_COL32(129, 140, 248, 255), [](const auto& s) { return s.cb2; }});
    channels.push_back({"SPI CS", ChannelFormat::DigitalBit, IM_COL32(232, 121, 249, 255), [](const auto& s) { return s.spiCs; }});
    channels.push_back({"SPI SCK", ChannelFormat::DigitalBit, IM_COL32(232, 121, 249, 255), [](const auto& s) { return s.spiClk; }});
    channels.push_back({"SPI MOSI", ChannelFormat::DigitalBit, IM_COL32(232, 121, 249, 255), [](const auto& s) { return s.spiMosi; }});
    channels.push_back({"SPI MISO", ChannelFormat::DigitalBit, IM_COL32(232, 121, 249, 255), [](const auto& s) { return s.spiMiso; }});
    channels.push_back({"UART TX", ChannelFormat::DigitalBit, IM_COL32(250, 204, 21, 255), [](const auto& s) { return s.uartTx; }});
    channels.push_back({"UART RX", ChannelFormat::DigitalBit, IM_COL32(250, 204, 21, 255), [](const auto& s) { return s.uartRx; }});

    const int numVisible = std::min(uiState.visibleCycles, static_cast<int>(count));
    constexpr float labelWidth = 84.0F;
    constexpr float rulerHeight = 22.0F;
    constexpr float trackHeight = 20.0F;
    constexpr float trackSpacing = 3.0F;

    float totalTracksHeight = rulerHeight + static_cast<float>(channels.size()) * (trackHeight + trackSpacing);

    ImGui::BeginChild("WaveformCanvasScroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 screenOrigin = ImGui::GetCursorScreenPos();
    float totalAvailWidth = std::max(ImGui::GetContentRegionAvail().x, 400.0F);
    float canvasWidth = totalAvailWidth - labelWidth - 16.0F;
    float pixelsPerCycle = canvasWidth / static_cast<float>(numVisible);
    float originX = screenOrigin.x + labelWidth;

    // Cache visible samples
    std::vector<Hardware::LogicSample> visibleSamples;
    visibleSamples.reserve(static_cast<size_t>(numVisible));
    for (int i = 0; i < numVisible; ++i) {
        size_t revOffset = static_cast<size_t>(uiState.scrollOffset + (numVisible - 1 - i));
        visibleSamples.push_back(analyzer.GetSample(revOffset));
    }

    // Ruler
    float rulerY = screenOrigin.y;
    drawList->AddRectFilled(ImVec2(originX, rulerY),
                            ImVec2(originX + canvasWidth, rulerY + rulerHeight),
                            IM_COL32(22, 26, 35, 255));
    drawList->AddLine(ImVec2(originX, rulerY + rulerHeight),
                      ImVec2(originX + canvasWidth, rulerY + rulerHeight),
                      IM_COL32(50, 58, 75, 255));

    // Tick marks
    int cycleStep = 1;
    constexpr std::array<int, 11> niceSteps = {1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500};
    for (int s : niceSteps) {
        if (static_cast<float>(s) * pixelsPerCycle >= 54.0F) {
            cycleStep = s;
            break;
        }
    }

    for (int i = 0; i < numVisible; ++i) {
        if (i % cycleStep == 0) {
            float tx = originX + static_cast<float>(i) * pixelsPerCycle;
            drawList->AddLine(ImVec2(tx, rulerY + rulerHeight - 7.0F),
                              ImVec2(tx, rulerY + rulerHeight),
                              IM_COL32(110, 125, 150, 255));
            drawList->AddLine(ImVec2(tx, rulerY + rulerHeight),
                              ImVec2(tx, rulerY + totalTracksHeight),
                              IM_COL32(255, 255, 255, 12));

            std::string tickLbl = std::format("+{}", i);
            drawList->AddText(ImVec2(tx + 2.0F, rulerY + 3.0F), IM_COL32(140, 155, 180, 255), tickLbl.c_str());
        }
    }

    // Channels
    float currentY = rulerY + rulerHeight + 2.0F;
    for (const auto& ch : channels) {
        // Label header
        drawList->AddRectFilled(ImVec2(screenOrigin.x, currentY),
                                ImVec2(originX - 2.0F, currentY + trackHeight),
                                IM_COL32(24, 28, 38, 255), 2.0F);
        ImVec2 tSz = ImGui::CalcTextSize(ch.name);
        drawList->AddText(ImVec2(screenOrigin.x + 4.0F, currentY + (trackHeight - tSz.y) * 0.5F),
                          IM_COL32(210, 218, 230, 255), ch.name);

        // Track background
        drawList->AddRectFilled(ImVec2(originX, currentY),
                                ImVec2(originX + canvasWidth, currentY + trackHeight),
                                IM_COL32(16, 20, 28, 140));

        if (ch.format == ChannelFormat::DigitalBit) {
            float yHigh = currentY + 3.0F;
            float yLow = currentY + trackHeight - 4.0F;
            drawList->AddLine(ImVec2(originX, yLow), ImVec2(originX + canvasWidth, yLow),
                              IM_COL32(35, 42, 56, 120));

            for (int i = 0; i < numVisible; ++i) {
                uint32_t val = ch.getValue(visibleSamples[static_cast<size_t>(i)]) & 1;
                float x1 = originX + static_cast<float>(i) * pixelsPerCycle;
                float x2 = originX + static_cast<float>(i + 1) * pixelsPerCycle;
                float y = (val != 0) ? yHigh : yLow;

                if (val != 0) {
                    drawList->AddRectFilled(ImVec2(x1, yHigh), ImVec2(x2, yLow),
                                            (ch.color & 0x00FFFFFF) | 0x22000000);
                }
                drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), ch.color, 1.5F);

                if (i > 0) {
                    uint32_t prevVal = ch.getValue(visibleSamples[static_cast<size_t>(i - 1)]) & 1;
                    if (prevVal != val) {
                        float prevY = (prevVal != 0) ? yHigh : yLow;
                        drawList->AddLine(ImVec2(x1, prevY), ImVec2(x1, y), ch.color, 1.5F);
                    }
                }
            }
        } else {
            // Hex Bus Bubble
            float yTop = currentY + 2.5F;
            float yBot = currentY + trackHeight - 3.5F;
            float yMid = (yTop + yBot) * 0.5F;

            int spanStart = 0;
            while (spanStart < numVisible) {
                uint32_t spanVal = ch.getValue(visibleSamples[static_cast<size_t>(spanStart)]);
                int spanEnd = spanStart;
                while (spanEnd + 1 < numVisible &&
                       ch.getValue(visibleSamples[static_cast<size_t>(spanEnd + 1)]) == spanVal) {
                    spanEnd++;
                }

                float x1 = originX + static_cast<float>(spanStart) * pixelsPerCycle;
                float x2 = originX + static_cast<float>(spanEnd + 1) * pixelsPerCycle;
                float indent = std::min(4.0F, (x2 - x1) * 0.35F);

                ImVec2 poly[6] = {
                    ImVec2(x1, yMid),
                    ImVec2(x1 + indent, yTop),
                    ImVec2(x2 - indent, yTop),
                    ImVec2(x2, yMid),
                    ImVec2(x2 - indent, yBot),
                    ImVec2(x1 + indent, yBot)
                };

                drawList->AddConvexPolyFilled(poly, 6, (ch.color & 0x00FFFFFF) | 0x24000000);
                drawList->AddPolyline(poly, 6, ch.color, ImDrawFlags_Closed, 1.2F);

                std::string hexText = (ch.format == ChannelFormat::Hex16)
                                          ? std::format("${:04X}", spanVal)
                                          : std::format("${:02X}", spanVal);
                ImVec2 htSz = ImGui::CalcTextSize(hexText.c_str());
                if ((x2 - x1) >= htSz.x + 6.0F) {
                    float textX = (x1 + x2 - htSz.x) * 0.5F;
                    float textY = (yTop + yBot - htSz.y) * 0.5F;
                    drawList->AddText(ImVec2(textX, textY), IM_COL32(240, 245, 255, 255), hexText.c_str());
                }

                spanStart = spanEnd + 1;
            }
        }

        currentY += trackHeight + trackSpacing;
    }

    // Mouse hover and cursor handling
    ImVec2 mousePos = ImGui::GetMousePos();
    bool mouseInCanvas = (mousePos.x >= originX && mousePos.x <= originX + canvasWidth &&
                          mousePos.y >= rulerY && mousePos.y <= currentY);

    if (mouseInCanvas) {
        int hCycle = static_cast<int>((mousePos.x - originX) / pixelsPerCycle);
        uiState.hoveredCycleOffset = std::clamp(hCycle, 0, numVisible - 1);

        // Vertical inspection line
        float hoverX = originX + static_cast<float>(uiState.hoveredCycleOffset) * pixelsPerCycle;
        drawList->AddLine(ImVec2(hoverX, rulerY), ImVec2(hoverX, currentY),
                          IM_COL32(255, 255, 255, 75), 1.0F);

        // Tooltip inspection
        const auto& sHover = visibleSamples[static_cast<size_t>(uiState.hoveredCycleOffset)];
        if (ImGui::BeginTooltip()) {
            ImGui::TextColored(ImVec4(0.22F, 0.74F, 0.97F, 1.0F),
                               "Cycle: #%llu (+%d in view)",
                               static_cast<unsigned long long>(sHover.cycle),
                               uiState.hoveredCycleOffset);
            ImGui::Separator();
            ImGui::Text("Address: $%04X", sHover.address);
            ImGui::Text("Data:    $%02X", sHover.data);
            ImGui::Text("R/W:     %s", sHover.rw ? "READ (1)" : "WRITE (0)");
            ImGui::Text("SYNC:    %d (%s)", sHover.sync, sHover.sync ? "Opcode Fetch" : "Operand");
            ImGui::Text("RDY:     %d", sHover.rdy);
            ImGui::Separator();
            ImGui::Text("IRQ: %d  |  NMI: %d", sHover.irq, sHover.nmi);
            ImGui::Text("VIA PA:  $%02X (0b%s)", sHover.portA, ToBinary8(sHover.portA).c_str());
            ImGui::Text("VIA PB:  $%02X (0b%s)", sHover.portB, ToBinary8(sHover.portB).c_str());
            ImGui::Text("CA1: %d  CA2: %d  CB1: %d  CB2: %d", sHover.ca1, sHover.ca2, sHover.cb1, sHover.cb2);
            ImGui::Separator();
            ImGui::Text("SPI:  CS:%d  CLK:%d  MOSI:%d  MISO:%d", sHover.spiCs, sHover.spiClk, sHover.spiMosi, sHover.spiMiso);
            ImGui::Text("UART: TX:%d  RX:%d", sHover.uartTx, sHover.uartRx);
            ImGui::EndTooltip();
        }

        // Dragging cursors
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            float xA = originX + static_cast<float>(uiState.cursorA) * pixelsPerCycle;
            float xB = originX + static_cast<float>(uiState.cursorB) * pixelsPerCycle;
            if (std::abs(mousePos.x - xA) <= 7.0F) {
                uiState.draggingCursor = 1;
            } else if (std::abs(mousePos.x - xB) <= 7.0F) {
                uiState.draggingCursor = 2;
            }
        }
    } else {
        uiState.hoveredCycleOffset = -1;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        uiState.draggingCursor = 0;
    } else if (uiState.draggingCursor != 0) {
        int draggedCyc = std::clamp(static_cast<int>((mousePos.x - originX) / pixelsPerCycle), 0, numVisible - 1);
        if (uiState.draggingCursor == 1) {
            uiState.cursorA = draggedCyc;
        } else if (uiState.draggingCursor == 2) {
            uiState.cursorB = draggedCyc;
        }
    }

    // Render Cursors
    if (uiState.enableCursors) {
        int cA = std::clamp(uiState.cursorA, 0, numVisible - 1);
        int cB = std::clamp(uiState.cursorB, 0, numVisible - 1);
        float xA = originX + static_cast<float>(cA) * pixelsPerCycle + pixelsPerCycle * 0.5F;
        float xB = originX + static_cast<float>(cB) * pixelsPerCycle + pixelsPerCycle * 0.5F;

        // Cursor A
        drawList->AddLine(ImVec2(xA, rulerY), ImVec2(xA, currentY), IM_COL32(250, 204, 21, 230), 1.5F);
        ImVec2 triA[3] = {ImVec2(xA - 6.0F, rulerY), ImVec2(xA + 6.0F, rulerY), ImVec2(xA, rulerY + 8.0F)};
        drawList->AddTriangleFilled(triA[0], triA[1], triA[2], IM_COL32(250, 204, 21, 255));
        drawList->AddText(ImVec2(xA - 3.0F, rulerY - 1.0F), IM_COL32(10, 10, 10, 255), "A");

        // Cursor B
        drawList->AddLine(ImVec2(xB, rulerY), ImVec2(xB, currentY), IM_COL32(56, 189, 248, 230), 1.5F);
        ImVec2 triB[3] = {ImVec2(xB - 6.0F, rulerY), ImVec2(xB + 6.0F, rulerY), ImVec2(xB, rulerY + 8.0F)};
        drawList->AddTriangleFilled(triB[0], triB[1], triB[2], IM_COL32(56, 189, 248, 255));
        drawList->AddText(ImVec2(xB - 3.0F, rulerY - 1.0F), IM_COL32(10, 10, 10, 255), "B");
    }

    ImGui::Dummy(ImVec2(totalAvailWidth, totalTracksHeight + 12.0F));
    ImGui::EndChild();
}

}  // namespace

void DrawLogicAnalyzerWindow(Control::AppState& state) {
    auto& analyzer = state.emulator.GetLogicAnalyzer();

    DrawTopBar(state, analyzer);

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    size_t count = analyzer.GetCount();

    // Determine sample to feed into IC Pinout
    Hardware::LogicSample inspectSample{};
    if (count > 0) {
        int numVis = std::min(uiState.visibleCycles, static_cast<int>(count));
        if (uiState.hoveredCycleOffset >= 0 && uiState.hoveredCycleOffset < numVis) {
            size_t revOff = static_cast<size_t>(uiState.scrollOffset + (numVis - 1 - uiState.hoveredCycleOffset));
            inspectSample = analyzer.GetSample(revOff);
        } else {
            size_t revOff = static_cast<size_t>(uiState.scrollOffset);
            inspectSample = analyzer.GetSample(revOff);
        }
    }

    if (uiState.viewMode == 0) {
        // Waveforms only
        DrawWaveforms(analyzer, contentSize.x, contentSize.y);
    } else if (uiState.viewMode == 1) {
        // IC Pinout only
        ImGui::BeginChild("PinoutOnlyScroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        DrawICPinout(inspectSample, contentSize.x, contentSize.y);
        ImGui::EndChild();
    } else {
        // Split View
        float pinoutWidth = std::clamp(contentSize.x * 0.38F, 280.0F, 360.0F);
        ImGui::BeginChild("PinoutSplitPane", ImVec2(pinoutWidth, 0), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        DrawICPinout(inspectSample, pinoutWidth - 16.0F, contentSize.y);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::BeginChild("WaveformSplitPane", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        DrawWaveforms(analyzer, contentSize.x - pinoutWidth - 24.0F, contentSize.y);
        ImGui::EndChild();
    }
}

}  // namespace GUI
