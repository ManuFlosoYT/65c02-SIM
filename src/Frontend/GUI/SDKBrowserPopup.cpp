#include "Frontend/GUI/SDKBrowserPopup.h"
#include "Frontend/Control/RomCartridgeLoaderService.h"

#include <imgui.h>
#include <algorithm>
#include <string>
#include <vector>

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#else
#include <iostream>
#endif

namespace GUI {

#ifdef TARGET_WASM
static void HandleSDKFileLoading(Control::AppState& state, const std::string& filename, const uint8_t* data, int size) {
    std::vector<uint8_t> dataVec(data, data + size);
    if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".65c") {
        Control::LoadCartridge(state, filename, dataVec);
    } else {
        Control::LoadBinaryRom(state, filename, dataVec);
    }
}
#endif

static void DrawSDKColumn(Control::AppState& state, const std::vector<std::string>& files, const std::string& subfolder) {
    for (const auto& file : files) {
        std::string displayName = file;
        if (displayName.ends_with(".65c") || displayName.ends_with(".c65")) {
            displayName.resize(displayName.size() - 4);
        }
        std::string label = displayName + "##" + file;
        if (ImGui::Selectable(label.c_str())) {
#ifdef TARGET_WASM
            std::string url = "roms/";
            if (subfolder != "cartridge") {
                url += subfolder;
                url += "/";
            }
            url += file;
            WebFileUtils::onFilePickedCallback = [&state, file](const char* filename, const uint8_t* data, int size) {
                HandleSDKFileLoading(state, filename, data, size);
            };
            WebFileUtils::fetch_file(url.c_str(), file.c_str());
#else
            std::string path = "SDK/";
            path += subfolder;
            path += "/";
            path += file;

            if (Control::LoadCartridge(state, path)) {
                ImGui::OpenPopup("Cartridge Loaded");
            } else {
                std::cerr << "SDK: Failed to load SDK cartridge " << file << "\n";
                ImGui::OpenPopup("ErrorLoadingROM");
            }
#endif
            ImGui::CloseCurrentPopup();
        }
    }
}

void DrawSDKBrowserPopup(Control::AppState& state) {
    if (state.sdk.showPopup) {
        ImGui::OpenPopup("SDK ROMs");
        state.sdk.showPopup = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr float verticalPadding = 40.0F;
    const float popupHeight = std::max(200.0F, viewport->WorkSize.y - (verticalPadding * 2.0F));
    const float popupWidth = std::min(800.0F, viewport->WorkSize.x - 40.0F);

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("SDK ROMs", nullptr, ImGuiWindowFlags_None)) {
        const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        bool canScrollDown = false;

        if (ImGui::BeginChild("SDKListRegion", ImVec2(0, -footerHeight), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
            if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
#ifdef TARGET_WASM
                ImGui::TextUnformatted("Loading ROM list...");
#else
                ImGui::TextUnformatted("No SDK resources found in 'SDK/' directory.");
                ImGui::TextUnformatted("Ensure SDK.zip was present at build time.");
#endif
            } else {
                ImGui::Columns(3, "SDKColumns", true);
                ImGui::Separator();
                ImGui::TextUnformatted("ROMs");
                ImGui::NextColumn();
                ImGui::TextUnformatted("MIDIs");
                ImGui::NextColumn();
                ImGui::TextUnformatted("VRAMs");
                ImGui::NextColumn();
                ImGui::Separator();

                DrawSDKColumn(state, state.sdk.roms, "cartridge");
                ImGui::NextColumn();
                DrawSDKColumn(state, state.sdk.midis, "midi");
                ImGui::NextColumn();
                DrawSDKColumn(state, state.sdk.vrams, "vram");
                ImGui::NextColumn();

                ImGui::Columns(1);
            }
            canScrollDown = ImGui::GetScrollY() < ImGui::GetScrollMaxY();
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        if (canScrollDown) {
            ImGui::SameLine();
            ImGui::TextDisabled("▼ Scroll down for more");
        }
        ImGui::EndPopup();
    }
}

}  // namespace GUI
