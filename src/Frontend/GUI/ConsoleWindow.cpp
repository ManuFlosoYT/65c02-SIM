#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ConsoleView.h"
#include "Frontend/GUI/Style/Style.h"
#include "Frontend/GUI/SDKBrowserPopup.h"
#include "Frontend/Control/RomCartridgeLoaderService.h"
#include "Frontend/Control/CartridgeUtils.h"

#include "Frontend/GUI/CustomFileDialog.h"
#include <SDL3/SDL.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "Hardware/Core/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <nlohmann/json.hpp>
#else
#include "Frontend/Control/SDKManager.h"
#endif

namespace GUI {

static void DrawLoadFileButton(AppState& state, bool cartLoaded) {
    const bool isLoaded = state.rom.loaded || cartLoaded;
    if (ImGui::Button(isLoaded ? "Eject File" : "Load File")) {
        if (isLoaded) {
            if (cartLoaded) {
                Control::PromptOrExecuteCartridgeAction(state, Control::PendingCartridgeAction::Eject);
            } else {
                state.rom.loaded = false;
                state.rom.bin = "";
                state.emulator.Reset();
                state.emulator.Pause();
            }
        } else {
#ifdef TARGET_WASM
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                std::vector<uint8_t> dataVec(data, data + size);
                std::string_view nameView(filename);
                if (nameView.ends_with(".65c") || nameView.ends_with(".c65")) {
                    // ImGui calls are invalid here (JS callback, outside a frame): defer to the draw loop
                    state.popups.showCartridgeLoaded = Control::LoadCartridge(state, filename, dataVec);
                } else {
                    if (!Control::LoadBinaryRom(state, filename, dataVec)) {
                        std::cerr << "Failed to load ROM\n";
                    }
                }
            };
            WebFileUtils::open_browser_file_picker(".bin,.65c,.c65");
#else
            if (!Frontend::CustomFileDialog::IsOpened()) {
                Frontend::CustomFileDialog::OpenDialog("ChooseFileDlgKey", "Choose File", "Supported Files (*.bin, *.65c){.bin,.65c,.c65},.bin,.65c,.*", ".");
            }
#endif
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (cartLoaded) {
            ImGui::TextUnformatted("Eject currently loaded cartridge");
        } else if (state.rom.loaded) {
            ImGui::TextUnformatted("Eject currently loaded ROM");
        } else {
            ImGui::TextUnformatted("Open file dialog to load a 65C02 binary ROM (.bin) or Cartridge (.65c)");
        }
        ImGui::EndTooltip();
    }
}

static void DrawSDKButton(AppState& state) {
    if (ImGui::Button("SDK ROMs")) {
#ifdef TARGET_WASM
        if (!state.sdk.loaded) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                try {
                    auto j = nlohmann::json::parse(std::string(std::bit_cast<const char*>(data), size));
                    state.sdk.roms = j.at("roms").get<std::vector<std::string>>();
                    state.sdk.midis = j.at("midis").get<std::vector<std::string>>();
                    state.sdk.vrams = j.at("vrams").get<std::vector<std::string>>();
                    state.sdk.loaded = true;
                    state.sdk.showPopup = true;
                } catch (...) {
                    std::cerr << "Failed to parse roms.json" << "\n";
                }
            };
            WebFileUtils::fetch_file("roms/roms.json", "roms.json");
        } else {
            state.sdk.showPopup = true;
        }
#else
        if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
            SDKManager::ScanExtractedSDK(state);
        }
        state.sdk.showPopup = true;
#endif
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Browse pre-compiled SDK sample ROMs and demos");
        ImGui::EndTooltip();
    }
}

#ifndef TARGET_WASM
static void DrawIDEButton(AppState& state, bool cartLoaded) {
    ImGui::BeginDisabled(cartLoaded || state.rom.loaded);
    if (ImGui::Button("Open IDE")) {
        state.ide.open = !state.ide.open;
        if (state.ide.code.empty()) {
            state.ide.code = "// Write your C or Assembly here!\n";
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (cartLoaded || state.rom.loaded) {
            ImGui::TextUnformatted("Eject cartridge/ROM first to use the IDE");
        } else {
            ImGui::TextUnformatted("Open integrated C/Assembly code editor and compiler");
        }
        ImGui::EndTooltip();
    }
}
#endif

static void DrawConsoleButtonBar(AppState& state) {
    bool cartLoaded = state.emulator.GetCartridge().loaded;

    DrawLoadFileButton(state, cartLoaded);

    ImGui::SameLine();
    DrawSDKButton(state);
    DrawSDKBrowserPopup(state);

    ImGui::SameLine();
    if (ImGui::Button("Copy Output")) {
        ImGui::OpenPopup("CopyConsoleOutput");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Copy console terminal text to clipboard");
        ImGui::EndTooltip();
    }

#ifndef TARGET_WASM
    ImGui::SameLine();
    DrawIDEButton(state, cartLoaded);
#endif

    ImGui::Separator();
}

static void DrawCopyOutputModal(AppState& state) {
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(displaySize.x * 0.75F, displaySize.y * 0.75F), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5F, displaySize.y * 0.5F), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("CopyConsoleOutput", nullptr, ImGuiWindowFlags_NoResize)) {
        static std::string fullText;
        if (ImGui::IsWindowAppearing()) {
            fullText.clear();
            for (const auto& line : Console::GetLines()) {
                fullText += line;
                fullText += "\n";
            }
            fullText += Console::GetCurrentLine();
        }

        const float buttonRowHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        ImGui::InputTextMultiline("##CopySource", fullText.data(), fullText.size() + 1,
                                  ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - buttonRowHeight),
                                  ImGuiInputTextFlags_ReadOnly);

        if (ImGui::Button("Close", ImVec2(120.0F * state.render.uiScale, 0))) {
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
    ImGui::SetNextWindowSize(ImVec2(state.emulation.gpuEnabled ? left_width : work_size.x, bottom_section_height),
                             ImGuiCond_Always);
    ImGui::Begin("Console", nullptr, window_flags);

    DrawConsoleButtonBar(state);
    DrawConsoleView(state);
    DrawCopyOutputModal(state);

    ImGui::End();
}

}  // namespace GUI
