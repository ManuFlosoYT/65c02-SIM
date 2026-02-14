#include "Frontend/GUI/UpdatePopup.h"

#include <SDL2/SDL.h>

#include <string>

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawUpdatePopup(AppState& state) {
    static bool updatePopupOpen = false;
    if (state.updateAvailable && !updatePopupOpen) {
        ImGui::OpenPopup("Update Available");
        updatePopupOpen = true;
    }

    // Always center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Update Available", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize)) {
        ImGui::Text("A new version of SIM_65C02 is available!");
        ImGui::Text("Latest Version: %s", state.latestVersionTag.c_str());
        ImGui::Text("Current Version: %s", PROJECT_VERSION);
        ImGui::Separator();

        ImGui::NewLine();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float btnWidth = (availWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        if (ImGui::Button("Download", ImVec2(btnWidth, 0))) {
            std::string url =
                "https://github.com/ManuFlosoYT/65c02-SIM/releases/latest";
            SDL_OpenURL(url.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Ignore", ImVec2(btnWidth, 0))) {
            state.updateAvailable = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace GUI
