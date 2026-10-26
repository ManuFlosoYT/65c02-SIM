#include "Frontend/GUI/Debugger/ICPinoutViewer.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace GUI {

namespace {

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

        bool viaSelected = ((sample.address & 0xFFF0) == 0x6000);
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
        bool aciaSelected = ((sample.address & 0xFFFC) == 0x5000);
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

}  // namespace

void ICPinoutViewer::Draw(int& selectedChip, const Hardware::LogicSample& sample,
                         float availWidth, float availHeight) {
    const char* chips[] = {"W65C02S (CPU)", "W65C22 (VIA)", "W65C51N (ACIA)"};
    float maxChipW = 0.0F;
    for (const char* ch : chips) {
        maxChipW = std::max(maxChipW, ImGui::CalcTextSize(ch).x);
    }
    float chipComboWidth = maxChipW + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0F + 16.0F;

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 10.0F, ImGui::GetCursorPosY() + 4.0F));
    ImGui::TextUnformatted("Chip:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(chipComboWidth);
    ImGui::Combo("##SelectIC", &selectedChip, chips, IM_ARRAYSIZE(chips));

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0F);
    ImGui::TextDisabled("Cycle: #%llu", static_cast<unsigned long long>(sample.cycle));
    ImGui::Spacing();

    std::vector<PinInfo> leftPins;
    std::vector<PinInfo> rightPins;
    PopulatePins(selectedChip, sample, leftPins, rightPins);

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

    float remainingPaneHeight = ImGui::GetContentRegionAvail().y;
    float chipY = canvasPos.y + 6.0F;
    if (remainingPaneHeight > chipHeight + 14.0F) {
        chipY = canvasPos.y + (remainingPaneHeight - chipHeight) * 0.5F;
    }

    drawList->AddRectFilled(ImVec2(chipX, chipY),
                            ImVec2(chipX + chipWidth, chipY + chipHeight),
                            IM_COL32(26, 29, 36, 255), 6.0F);
    drawList->AddRect(ImVec2(chipX, chipY),
                      ImVec2(chipX + chipWidth, chipY + chipHeight),
                      IM_COL32(65, 72, 88, 255), 6.0F, 0, 1.5F);

    drawList->AddCircleFilled(ImVec2(centerX, chipY), 7.5F, IM_COL32(18, 20, 25, 255));
    drawList->AddCircle(ImVec2(centerX, chipY), 7.5F, IM_COL32(65, 72, 88, 255), 16, 1.2F);

    drawList->AddCircleFilled(ImVec2(chipX + 11.0F, chipY + 14.0F), 3.0F, IM_COL32(45, 50, 62, 255));

    const char* chipTitle = (selectedChip == 0) ? "W65C02S" : (selectedChip == 1 ? "W65C22" : "W65C51N");
    float textCenterY = chipY + chipHeight * 0.5F;
    AddTextRotated(drawList, ImVec2(centerX, textCenterY), 1.5707963F, IM_COL32(220, 225, 235, 255), chipTitle);

    ImVec2 mousePos = ImGui::GetMousePos();

    for (size_t i = 0; i < leftPins.size(); i++) {
        const auto& pin = leftPins[i];
        float py = chipY + 22.0F + static_cast<float>(i) * pinPitch;

        drawList->AddRectFilled(ImVec2(chipX - stubLen, py - stubThick * 0.5F),
                                ImVec2(chipX, py + stubThick * 0.5F),
                                IM_COL32(175, 182, 195, 255), 1.0F);

        std::string numStr = std::to_string(pin.number);
        drawList->AddText(ImVec2(chipX + 4.0F, py - 7.0F), IM_COL32(130, 140, 160, 255), numStr.c_str());

        ImVec2 lblSz = ImGui::CalcTextSize(pin.label);
        float lblX = chipX - stubLen - 6.0F - lblSz.x;
        drawList->AddText(ImVec2(lblX, py - 7.0F), IM_COL32(230, 235, 245, 255), pin.label);

        float ledX = lblX - 8.0F - ledRadius;
        ImU32 ledCol = (pin.state == 1) ? IM_COL32(50, 220, 50, 255)
                                        : (pin.state == 0 ? IM_COL32(70, 70, 70, 255) : IM_COL32(120, 120, 120, 255));
        drawList->AddCircleFilled(ImVec2(ledX, py), ledRadius, ledCol);
        drawList->AddCircle(ImVec2(ledX, py), ledRadius, IM_COL32(20, 20, 20, 255), 12, 1.0F);

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

    for (size_t i = 0; i < rightPins.size(); i++) {
        const auto& pin = rightPins[i];
        float py = chipY + 22.0F + static_cast<float>(i) * pinPitch;

        drawList->AddRectFilled(ImVec2(chipX + chipWidth, py - stubThick * 0.5F),
                                ImVec2(chipX + chipWidth + stubLen, py + stubThick * 0.5F),
                                IM_COL32(175, 182, 195, 255), 1.0F);

        std::string numStr = std::to_string(pin.number);
        ImVec2 numSz = ImGui::CalcTextSize(numStr.c_str());
        drawList->AddText(ImVec2(chipX + chipWidth - numSz.x - 4.0F, py - 7.0F), IM_COL32(130, 140, 160, 255), numStr.c_str());

        float lblX = chipX + chipWidth + stubLen + 6.0F;
        drawList->AddText(ImVec2(lblX, py - 7.0F), IM_COL32(230, 235, 245, 255), pin.label);
        ImVec2 lblSz = ImGui::CalcTextSize(pin.label);

        float ledX = lblX + lblSz.x + 8.0F + ledRadius;
        ImU32 ledCol = (pin.state == 1) ? IM_COL32(50, 220, 50, 255)
                                        : (pin.state == 0 ? IM_COL32(70, 70, 70, 255) : IM_COL32(120, 120, 120, 255));
        drawList->AddCircleFilled(ImVec2(ledX, py), ledRadius, ledCol);
        drawList->AddCircle(ImVec2(ledX, py), ledRadius, IM_COL32(20, 20, 20, 255), 12, 1.0F);

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

    float totalNeeded = (chipY + chipHeight + 14.0F) - canvasPos.y;
    if (totalNeeded > remainingPaneHeight) {
        ImGui::Dummy(ImVec2(canvasWidth, totalNeeded));
    }
}

}  // namespace GUI
