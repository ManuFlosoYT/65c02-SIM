#include "Frontend/GUI/BackendErrorPopup.h"

#include <imgui.h>
#include <string>

#include "Frontend/Control/AppState.h"
#include "Frontend/Platform/PlatformUtils.h"

namespace Frontend {

void DrawBackendErrorPopup(Control::AppState& state) {
    if (!state.lastBackendError.active) {
        return;
    }

    constexpr const char* popupId = "Backend Error";
    if (!ImGui::IsPopupOpen(popupId)) {
        ImGui::OpenPopup(popupId);
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "A backend error has been detected:");
        ImGui::Spacing();

        ImGui::TextWrapped("%s", state.lastBackendError.summary.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Stack Trace:");
        const float uiScale = state.render.uiScale > 0.0F ? state.render.uiScale : 1.0F;
        ImVec2 boxSize(560.0F * uiScale, 180.0F * uiScale);

        std::string traceText = state.lastBackendError.stackTrace.empty()
                                    ? "No stack trace available."
                                    : state.lastBackendError.stackTrace;

        ImGui::InputTextMultiline("##error_stacktrace", traceText.data(), traceText.size(),
                                  boxSize, ImGuiInputTextFlags_ReadOnly);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Copy Error")) {
            std::string payload = "Summary: " + state.lastBackendError.summary +
                                  "\n\nStack Trace:\n" + state.lastBackendError.stackTrace;
            ImGui::SetClipboardText(payload.c_str());
        }

        ImGui::SameLine();
        if (ImGui::Button("Report Issue")) {
            Frontend::Platform::OpenURL("https://github.com/ManuFlosoYT/65c02-SIM/issues");
        }

        ImGui::SameLine();
        if (ImGui::Button("Restart Backend")) {
            if (state.RecreateBackend(state.cachedSidSampleRate)) {
                state.lastBackendError.Clear();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Exit Application")) {
            state.requestExit = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}  // namespace Frontend
