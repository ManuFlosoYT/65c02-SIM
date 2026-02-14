#include "Frontend/GUI/ConsoleWindow.h"

#include <ImGuiFileDialog.h>

#include <string>
#include <vector>

#include "Frontend/Control/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawConsoleWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                       float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float left_width = work_size.x * 0.5f;
    float bottom_section_height = work_size.y - top_section_height;

    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + top_section_height),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(state.gpuEnabled ? left_width : work_size.x,
                                    bottom_section_height),
                             ImGuiCond_Always);
    ImGui::Begin("Console", nullptr, window_flags);

    if (ImGui::Button("Load ROM")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
                                                    "Choose File", ".bin", ".");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy Output")) {
        ImGui::OpenPopup("CopyConsoleOutput");
    }
    ImGui::Separator();

    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused()) {
        for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
            unsigned int c = io.InputQueueCharacters[n];
            if (c > 0 && c < 0x80) {
                if (c == '\r' || c == '\n') continue;
                state.emulator.InjectKey((char)c);
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
            state.emulator.InjectKey(0x7F);
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            state.emulator.InjectKey('\r');
    }

    for (const auto& line : Console::consoleLines) {
        ImGui::TextUnformatted(line.c_str());
    }
    ImGui::TextUnformatted(Console::currentLine.c_str());

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
        ImGui::SetScrollHereY(1.0f);

    // Copy Output Modal
    if (ImGui::BeginPopupModal("CopyConsoleOutput", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        static std::string fullText;
        if (fullText.empty() || fullText.length() < 10) {
            fullText.clear();
            for (const auto& line : Console::consoleLines)
                fullText += line + "\n";
            fullText += Console::currentLine;
        }

        std::vector<char> buffer(fullText.begin(), fullText.end());
        buffer.push_back(0);

        ImGui::InputTextMultiline("##CopySource", buffer.data(), buffer.size(),
                                  ImVec2(800, 600),
                                  ImGuiInputTextFlags_ReadOnly);

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            fullText.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

}  // namespace GUI
