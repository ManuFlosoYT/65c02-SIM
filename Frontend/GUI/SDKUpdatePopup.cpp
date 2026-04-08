#include "Frontend/GUI/SDKUpdatePopup.h"
#include "Frontend/Control/SDKManager.h"

#include <imgui.h>
#include <string>

namespace GUI {

void DrawSDKUpdatePopup(AppState& state) {
    if (!state.sdk.showUpdatePopup) {
        return;
    }

    if (!ImGui::IsPopupOpen("SDK Update Available")) {
        ImGui::OpenPopup("SDK Update Available");
    }

    // Always center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal(
            "SDK Update Available", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        
        ImGui::TextUnformatted("The extracted SDK is outdated or missing manifest info.");
        ImGui::NewLine();
        ImGui::TextUnformatted(("Extracted Version: " + (state.sdk.extractedVersion.empty() ? "Unknown" : state.sdk.extractedVersion)).c_str());
        ImGui::TextUnformatted(("Current Version:   " + std::string(PROJECT_VERSION)).c_str());
        ImGui::NewLine();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.8F, 0.0F, 1.0F)); // Warning color
        ImGui::PushTextWrapPos(400.0F);
        ImGui::TextUnformatted("WARNING: Updating will completely DELETE the 'SDK/' directory and its contents before re-extracting. Any custom files stored there will be lost.");
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::NewLine();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float btnWidth = (availWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5F;

        if (ImGui::Button("Update SDK", ImVec2(btnWidth, 0))) {
            SDKManager::DeleteSDK();
            SDKManager::ExtractBundledSDK();
            SDKManager::ScanExtractedSDK(state);
            state.sdk.showUpdatePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Ignore", ImVec2(btnWidth, 0))) {
            SDKManager::ScanExtractedSDK(state); // Clean fallback: use what we have
            state.sdk.showUpdatePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace GUI
