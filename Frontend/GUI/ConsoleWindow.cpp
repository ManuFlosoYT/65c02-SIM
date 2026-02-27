#include "Frontend/GUI/ConsoleWindow.h"

#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "Frontend/Control/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawConsoleButtonBar() {
    if (ImGui::Button("Load ROM")) {
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy Output")) {
        ImGui::OpenPopup("CopyConsoleOutput");
    }
    ImGui::Separator();
}

static void HandleConsoleInput(AppState& state) {
    ImGuiIO& imgui_io = ImGui::GetIO();
    if (ImGui::IsWindowFocused()) {
        SDL_Window* sdl_window = SDL_GetKeyboardFocus();
        if (sdl_window != nullptr) {
            SDL_StartTextInput(sdl_window);
        }
        for (int idx = 0; idx < imgui_io.InputQueueCharacters.Size; idx++) {
            unsigned int chr = imgui_io.InputQueueCharacters[idx];
            if (chr > 0 && chr < 0x80) {
                if (chr == '\r' || chr == '\n') {
                    continue;
                }
                state.emulator.InjectKey((char)chr);
            }
        }

        /* Handle special keys not in characters queue */
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            state.emulator.InjectKey(0x1B);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            state.emulator.InjectKey(0x09);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            state.emulator.InjectKey(0x7F);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            state.emulator.InjectKey('\r');
        }

        /* Handle Ctrl + A-Z */
        if (imgui_io.KeyCtrl) {
            for (int key = ImGuiKey_A; key <= ImGuiKey_Z; key++) {
                if (ImGui::IsKeyPressed((ImGuiKey)key)) {
                    state.emulator.InjectKey((char)(1 + (key - ImGuiKey_A)));
                }
            }
        }
    }
}

static void DrawConsoleText() {
    for (const auto& line : Console::GetLines()) {
        ImGui::TextUnformatted(line.c_str());
    }
    ImGui::TextUnformatted(Console::GetCurrentLine().c_str());

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20) {
        ImGui::SetScrollHereY(1.0F);
    }
}

static void DrawCopyOutputModal() {
    if (ImGui::BeginPopupModal("CopyConsoleOutput", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static std::string fullText;
        if (fullText.empty() || fullText.length() < 10) {
            fullText.clear();
            for (const auto& line : Console::GetLines()) {
                fullText += line + "\n";
            }
            fullText += Console::GetCurrentLine();
        }

        std::vector<char> buffer(fullText.begin(), fullText.end());
        buffer.push_back(0);

        ImGui::InputTextMultiline("##CopySource", buffer.data(), buffer.size(), ImVec2(800, 600),
                                  ImGuiInputTextFlags_ReadOnly);

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            fullText.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawConsoleWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float left_width = work_size.x * 0.5F;
    float bottom_section_height = work_size.y - top_section_height;

    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + top_section_height), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(state.gpuEnabled ? left_width : work_size.x, bottom_section_height),
                             ImGuiCond_Always);
    ImGui::Begin("Console", nullptr, window_flags);

    DrawConsoleButtonBar();
    HandleConsoleInput(state);
    DrawConsoleText();
    DrawCopyOutputModal();

    ImGui::End();
}

}  // namespace GUI
