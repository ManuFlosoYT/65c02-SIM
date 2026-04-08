#include "Frontend/UI/UIModules.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/RegistersWindow.h"
#include "Frontend/GUI/ScriptConsoleWindow.h"
#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/GUI/UpdatePopup.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Frontend/GUI/IDEWindow.h"

using namespace Control;
using namespace Core;

namespace Frontend {

static void DrawROMAndStatePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorLoadingROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error loading ROM. Please check the file.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorSavingState", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error saving state. Please check your permissions.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SavestateFeedback", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        auto result = state.emulator.GetLastLoadResult();
        if (result == SavestateLoadResult::VersionMismatch || result == SavestateLoadResult::HashMismatch) {
            ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "Warning: Savestate compatibility issue");
            if (result == SavestateLoadResult::VersionMismatch) {
                ImGui::TextUnformatted("Version mismatch detected:");
                ImGui::BulletText("Saved: %s", state.emulator.GetLastLoadVersion().c_str());
                ImGui::BulletText("Current: %s", PROJECT_VERSION);
            } else {
                ImGui::TextUnformatted(
                    "Hash mismatch. The data might be modified or "
                    "corrupt.");
            }
            ImGui::TextUnformatted(
                "The state was loaded, but some things might not work "
                "correctly.");
        } else if (result == SavestateLoadResult::StructuralError) {
            ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "Error: Failed to load state");
            ImGui::TextUnformatted("The data is structurally incompatible or corrupted.");
        } else {
            ImGui::TextUnformatted("An unknown error occurred while loading the state.");
        }

        ImGui::Spacing();
        ImGui::SetNextWindowSize(ImVec2(120, 0));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawSDCardPopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (state.popups.sdCardDisabled) {
        ImGui::OpenPopup("SD Card Disabled");
        state.popups.sdCardDisabled = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Disabled", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The SD Card device is currently disabled in the Memory Layout.");
        ImGui::TextUnformatted("Please enable it in the Memory Layout (Debugger -> Memory Layout) to use it.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.sdCardWebWarning) {
        ImGui::OpenPopup("SD Card Web Warning");
        state.popups.sdCardWebWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Web Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("SD Card Mounted (Web Port)");
        ImGui::Separator();
        ImGui::TextUnformatted("Due to browser security limitations, changes made to the SD card");
        ImGui::TextUnformatted("are stored in a virtual filesystem and NOT synced to your local file.");
        ImGui::Spacing();
        ImGui::TextUnformatted("To save your changes, you MUST use 'Unmount & Save (Download)'");
        ImGui::TextUnformatted("or the 'Save Changes' button in the Control Window.");
        ImGui::Spacing();
        if (ImGui::Button("I Understand", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawCartridgePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal("Cartridge Loaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        const auto& cart = state.emulator.GetCartridge();
        if (cart.loaded) {
            std::string nameLine = "Name: " + cart.metadata.name;
            std::string authorLine = "Author: " + cart.metadata.author;
            std::string versionLine = "Version: " + cart.metadata.version;
            ImGui::TextUnformatted(nameLine.c_str());
            ImGui::TextUnformatted(authorLine.c_str());
            ImGui::TextUnformatted(versionLine.c_str());
            ImGui::Separator();
            ImGui::TextUnformatted(cart.metadata.description.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawPopups(AppState& state) {
    DrawROMAndStatePopups(state);
    DrawSDCardPopups(state);
    DrawCartridgePopups(state);
}

void DrawGUIWindows(AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, unsigned int windowFlags) {
    GUI::DrawUpdatePopup(state);
    GUI::DrawControlWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawLCDWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawRegistersWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawConsoleWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawScriptConsoleWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    state.crt.time = static_cast<float>(SDL_GetTicks()) / 1000.0F;
    GUI::DrawVRAMViewerWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawIDEWindow(state);
}

}  // namespace Frontend
