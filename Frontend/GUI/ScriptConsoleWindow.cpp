#include "Frontend/GUI/ScriptConsoleWindow.h"

#include "Frontend/Control/AppState.h"
#include "Hardware/Core/Emulator.h"

namespace GUI {

void DrawScriptConsoleWindow(Control::AppState& state, const ImVec2& work_pos, const ImVec2& work_size,
                             float top_section_height, ImGuiWindowFlags window_flags) {
    if (!state.script.showConsole) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Script Console", &state.script.showConsole, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::Button("Clear")) {
            state.emulator.GetScriptEngine().ClearOutput();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            state.script.showConsole = false;
        }

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        
        auto output = state.emulator.GetScriptEngine().GetOutput();
        for (const auto& line : output) {
            ImGui::TextUnformatted(line.c_str());
        }
        
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0F);
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

}  // namespace GUI
