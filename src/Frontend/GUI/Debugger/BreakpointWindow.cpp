#include "Frontend/GUI/Debugger/BreakpointWindow.h"

#include <imgui.h>

#include <array>
#include <iomanip>
#include <sstream>

#include "Hardware/Core/BreakpointManager.h"

namespace GUI {

namespace {

constexpr std::array<const char*, 5> kTypeNames = {"PC Address", "Register", "Flag", "Memory Value", "Watchpoint"};
constexpr std::array<const char*, 6> kOpNames = {"==", "!=", "<", "<=", ">", ">="};
constexpr std::array<const char*, 5> kRegNames = {"A", "X", "Y", "SP", "PC"};
constexpr std::array<const char*, 7> kFlagNames = {"C", "Z", "I", "D", "B", "V", "N"};

struct BreakpointFormState {
    int typeIndex = 0;
    int regIndex = 0;
    int flagIndex = 0;
    int opIndex = 0;
    uint16_t address = 0;
    uint16_t value = 0;
    int compoundOpIndex = 0;
    bool hitOnce = false;
    std::array<char, 64> labelBuf = {{0}};
    std::vector<Hardware::BreakCondition> conditions;
};

std::string ConditionToString(const Hardware::BreakCondition& cond) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    switch (cond.type) {
        case Hardware::BreakpointType::PCAddress:
            oss << "PC == 0x" << std::setw(4) << cond.address;
            break;
        case Hardware::BreakpointType::RegisterCondition:
            oss << kRegNames.at(static_cast<int>(cond.reg)) << " " << kOpNames.at(static_cast<int>(cond.op)) << " 0x"
                << std::setw(cond.reg >= Hardware::RegisterId::SP ? 4 : 2) << cond.value;
            break;
        case Hardware::BreakpointType::FlagCondition:
            oss << kFlagNames.at(static_cast<int>(cond.flag)) << " " << kOpNames.at(static_cast<int>(cond.op)) << " "
                << cond.value;
            break;
        case Hardware::BreakpointType::MemoryCondition:
            oss << "[0x" << std::setw(4) << cond.address << "] " << kOpNames.at(static_cast<int>(cond.op)) << " 0x"
                << std::setw(2) << cond.value;
            break;
        case Hardware::BreakpointType::MemoryWatchpoint:
            oss << "write @ 0x" << std::setw(4) << cond.address;
            break;
    }
    return oss.str();
}

std::string BreakpointSummary(const Hardware::Breakpoint& bkpt) {
    std::string result;
    const char* joiner = (bkpt.compoundOp == Hardware::LogicOp::And) ? " AND " : " OR ";
    for (size_t index = 0; index < bkpt.conditions.size(); index++) {
        if (index > 0) {
            result += joiner;
        }
        result += ConditionToString(bkpt.conditions[index]);
    }
    return result;
}

Hardware::BreakCondition BuildConditionFromForm(const BreakpointFormState& form) {
    Hardware::BreakCondition cond;
    cond.type = static_cast<Hardware::BreakpointType>(form.typeIndex);
    cond.reg = static_cast<Hardware::RegisterId>(form.regIndex);
    cond.flag = static_cast<Hardware::FlagId>(form.flagIndex);
    cond.op = static_cast<Hardware::CompareOp>(form.opIndex);
    cond.address = form.address;
    cond.value = form.value;
    return cond;
}

void DrawConditionForm(BreakpointFormState& form) {
    ImGui::PushItemWidth(160);
    ImGui::Combo("Type", &form.typeIndex, kTypeNames.data(), static_cast<int>(kTypeNames.size()));
    ImGui::PopItemWidth();

    auto typeEnum = static_cast<Hardware::BreakpointType>(form.typeIndex);

    ImGui::PushItemWidth(100);
    switch (typeEnum) {
        case Hardware::BreakpointType::PCAddress:
            ImGui::InputScalar("Address##cond", ImGuiDataType_U16, &form.address, nullptr, nullptr, "%04X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            break;

        case Hardware::BreakpointType::RegisterCondition:
            ImGui::Combo("Register", &form.regIndex, kRegNames.data(), static_cast<int>(kRegNames.size()));
            ImGui::SameLine();
            ImGui::Combo("Op##reg", &form.opIndex, kOpNames.data(), static_cast<int>(kOpNames.size()));
            ImGui::SameLine();
            ImGui::InputScalar("Value##reg", ImGuiDataType_U16, &form.value, nullptr, nullptr, "%04X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            break;

        case Hardware::BreakpointType::FlagCondition:
            ImGui::Combo("Flag", &form.flagIndex, kFlagNames.data(), static_cast<int>(kFlagNames.size()));
            ImGui::SameLine();
            ImGui::Combo("Op##flag", &form.opIndex, kOpNames.data(), static_cast<int>(kOpNames.size()));
            ImGui::SameLine();
            ImGui::InputScalar("Value##flag", ImGuiDataType_U16, &form.value, nullptr, nullptr, "%X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            break;

        case Hardware::BreakpointType::MemoryCondition:
            ImGui::InputScalar("Address##mem", ImGuiDataType_U16, &form.address, nullptr, nullptr, "%04X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            ImGui::Combo("Op##mem", &form.opIndex, kOpNames.data(), static_cast<int>(kOpNames.size()));
            ImGui::SameLine();
            ImGui::InputScalar("Value##mem", ImGuiDataType_U16, &form.value, nullptr, nullptr, "%02X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            break;

        case Hardware::BreakpointType::MemoryWatchpoint:
            ImGui::InputScalar("Address##wp", ImGuiDataType_U16, &form.address, nullptr, nullptr, "%04X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            break;
    }
    ImGui::PopItemWidth();
}

void DrawHelperSection(BreakpointFormState& form) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("Quick Breakpoints");
    ImGui::PopStyleColor();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));

    if (ImGui::Button("PC == addr")) {
        form.typeIndex = 0;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("A == val")) {
        form.typeIndex = 1;
        form.regIndex = 0;
        form.opIndex = 0;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("X == val")) {
        form.typeIndex = 1;
        form.regIndex = 1;
        form.opIndex = 0;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Y == val")) {
        form.typeIndex = 1;
        form.regIndex = 2;
        form.opIndex = 0;
        form.conditions.clear();
    }

    if (ImGui::Button("[addr] == val")) {
        form.typeIndex = 3;
        form.opIndex = 0;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Write @ addr")) {
        form.typeIndex = 4;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Z flag set")) {
        form.typeIndex = 2;
        form.flagIndex = 1;
        form.opIndex = 0;
        form.value = 1;
        form.conditions.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("C flag set")) {
        form.typeIndex = 2;
        form.flagIndex = 0;
        form.opIndex = 0;
        form.value = 1;
        form.conditions.clear();
    }

    ImGui::PopStyleVar();
}

void DrawConditionList(BreakpointFormState& form) {
    if (form.conditions.empty()) {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted("Pending conditions:");
    ImGui::PopStyleColor();

    int removeIdx = -1;
    for (size_t idx = 0; idx < form.conditions.size(); idx++) {
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextUnformatted(ConditionToString(form.conditions[idx]).c_str());
        ImGui::SameLine();
        std::string btnLabel = "X##cond" + std::to_string(idx);
        if (ImGui::SmallButton(btnLabel.c_str())) {
            removeIdx = static_cast<int>(idx);
        }
    }
    if (removeIdx >= 0) {
        form.conditions.erase(form.conditions.begin() + removeIdx);
    }
}

void DrawBreakpointList(Hardware::BreakpointManager& manager) {
    const auto bpList = manager.GetBreakpointsSnapshot();

    if (bpList.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted("No breakpoints defined.");
        ImGui::PopStyleColor();
        return;
    }

    if (ImGui::BeginTable("BreakpointTable", 6,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, ImGui::GetContentRegionAvail().y * 0.4F))) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0F);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0F);
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("Conditions", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Hits", ImGuiTableColumnFlags_WidthFixed, 45.0F);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0F);
        ImGui::TableHeadersRow();

        uint32_t removeId = 0;
        for (const auto& entry : bpList) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            bool enabled = entry.enabled;
            std::string cbLabel = "##en" + std::to_string(entry.id);
            if (ImGui::Checkbox(cbLabel.c_str(), &enabled)) {
                manager.SetEnabled(entry.id, enabled);
            }

            ImGui::TableNextColumn();
            std::string idText = "#" + std::to_string(entry.id);
            ImGui::TextUnformatted(idText.c_str());

            ImGui::TableNextColumn();
            if (entry.label.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextUnformatted("-");
                ImGui::PopStyleColor();
            } else {
                ImGui::TextUnformatted(entry.label.c_str());
            }

            ImGui::TableNextColumn();
            std::string summary = BreakpointSummary(entry);
            ImGui::TextUnformatted(summary.c_str());

            ImGui::TableNextColumn();
            std::string hitText = std::to_string(entry.hitCount);
            ImGui::TextUnformatted(hitText.c_str());

            ImGui::TableNextColumn();
            std::string delLabel = "X##del" + std::to_string(entry.id);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.3F, 0.3F, 1.0F));
            if (ImGui::SmallButton(delLabel.c_str())) {
                removeId = entry.id;
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndTable();

        if (removeId > 0) {
            manager.RemoveBreakpoint(removeId);
        }
    }
}

}  // namespace

void DrawBreakpointWindow(Control::AppState& state) {
    static BreakpointFormState form;
    auto& manager = state.emulator.GetBreakpointManager();
    const auto bpSnapshot = manager.GetBreakpointsSnapshot();

    int activeCount = 0;
    for (const auto& entry : bpSnapshot) {
        if (entry.enabled) {
            activeCount++;
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("Breakpoints");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    std::string activeText = "(" + std::to_string(activeCount) + " active)";
    ImGui::TextUnformatted(activeText.c_str());
    ImGui::PopStyleColor();
    float clearBtnWidth = ImGui::CalcTextSize("Clear All").x + (ImGui::GetStyle().FramePadding.x * 2.0F);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - clearBtnWidth + ImGui::GetCursorPosX() -
                    ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::Button("Clear All") && !bpSnapshot.empty()) {
        manager.ClearAll();
    }

    ImGui::Separator();

    DrawBreakpointList(manager);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawHelperSection(form);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("New Breakpoint");
    ImGui::PopStyleColor();

    DrawConditionForm(form);

    ImGui::Spacing();

    if (ImGui::Button("Add Condition")) {
        form.conditions.push_back(BuildConditionFromForm(form));
    }
    ImGui::SameLine();

    constexpr std::array<const char*, 2> logicOps = {"AND", "OR"};
    ImGui::PushItemWidth(75);
    ImGui::Combo("Logic##compound", &form.compoundOpIndex, logicOps.data(), static_cast<int>(logicOps.size()));
    ImGui::PopItemWidth();

    DrawConditionList(form);

    ImGui::Spacing();

    ImGui::Checkbox("Hit Once (auto-disable)", &form.hitOnce);

    ImGui::PushItemWidth(200);
    ImGui::InputText("Label", form.labelBuf.data(), form.labelBuf.size());
    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (ImGui::Button("Create Breakpoint")) {
        Hardware::Breakpoint newBp;
        newBp.hitOnce = form.hitOnce;
        newBp.compoundOp = static_cast<Hardware::LogicOp>(form.compoundOpIndex);
        newBp.label = std::string(form.labelBuf.data());

        if (form.conditions.empty()) {
            form.conditions.push_back(BuildConditionFromForm(form));
        }
        newBp.conditions = form.conditions;

        manager.AddBreakpoint(std::move(newBp));

        form.conditions.clear();
        form.hitOnce = false;
        form.labelBuf.fill(0);
    }
}

}  // namespace GUI
