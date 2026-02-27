#include "Frontend/GUI/UpdatePopup.h"

#include <SDL3/SDL.h>

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
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal(
            "Update Available", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("A new version of SIM_65C02 is available!");
        ImGui::TextUnformatted(("Latest Version: " + state.latestVersionTag).c_str());
        ImGui::TextUnformatted(("Current Version: " + std::string(PROJECT_VERSION)).c_str());
        ImGui::Separator();

        ImGui::NewLine();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float btnWidth = (availWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5F;

        if (ImGui::Button("Download", ImVec2(btnWidth, 0))) {
            std::string url = "https://github.com/ManuFlosoYT/65c02-SIM/releases/latest";
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
