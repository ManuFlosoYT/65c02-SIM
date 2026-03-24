#include "Frontend/GUI/ConsoleWindow.h"

#include <ImGuiFileDialog.h>
#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "Frontend/Control/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <fstream>
#include <nlohmann/json.hpp>
#endif

namespace GUI {

#ifdef TARGET_WASM
static void DrawSDKPopup(AppState& state) {
    if (state.sdk.showPopup) {
        ImGui::OpenPopup("SDK ROMs");
        state.sdk.showPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("SDK ROMs", nullptr, ImGuiWindowFlags_None)) {
        if (state.sdk.roms.empty() && state.sdk.midis.empty() && state.sdk.vrams.empty()) {
            ImGui::TextUnformatted("Loading ROM list...");
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

            auto draw_column = [&](const std::vector<std::string>& files) {
                for (const auto& file : files) {
                    if (ImGui::Selectable(file.c_str())) {
                        std::string url = "roms/" + file;
                        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data,
                                                                      int size) {
                            state.rom.data.assign(data, data + size);
                            state.emulator.Pause();
                            std::string errorMsg;
                            if (state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), filename,
                                                              errorMsg)) {
                                state.rom.bin = filename;
                                state.rom.loaded = true;
                                state.rom.symbols.Clear();
                                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                                state.emulator.ClearProfiler();
                            }
                        };
                        WebFileUtils::fetch_file(url.c_str(), file.c_str());
                        ImGui::CloseCurrentPopup();
                    }
                }
            };

            draw_column(state.sdk.roms);
            ImGui::NextColumn();
            draw_column(state.sdk.midis);
            ImGui::NextColumn();
            draw_column(state.sdk.vrams);
            ImGui::NextColumn();

            ImGui::Columns(1);
        }
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
#endif

static void DrawConsoleButtonBar(AppState& state) {
    bool cartLoaded = state.emulator.GetCartridge().loaded;
    ImGui::BeginDisabled(cartLoaded);
    if (ImGui::Button("Load ROM")) {
#ifdef TARGET_WASM
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            state.rom.data.assign(data, data + size);
            state.emulator.Pause();
            std::string errorMsg;
            if (state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), filename, errorMsg)) {
                state.rom.bin = filename;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                state.emulator.ClearProfiler();
                state.emulator.ClearCartridge();
            } else {
                printf("Failed to load ROM: %s\n", errorMsg.c_str());
            }
        };
        WebFileUtils::open_browser_file_picker(".bin");
#else
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
        }
#endif
    }
    ImGui::EndDisabled();
    if (cartLoaded && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Eject cartridge first to load a different ROM");
            ImGui::EndTooltip();
        }
    }

    ImGui::SameLine();
    const char* cartBtnText = cartLoaded ? "Eject Cartridge" : "Load Cartridge (.65c)";
    if (ImGui::Button(cartBtnText)) {
        if (cartLoaded) {
            state.emulator.ClearCartridge();
        } else {
#ifdef TARGET_WASM
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            // Write to virtual FS
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }
            ImGuiFileDialog::Instance()->OpenDialog("ChooseCartridgeDlgKey", "Load Cartridge", ".65c", ".");
        };
        WebFileUtils::open_browser_file_picker(".65c");
#else
        if (!ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseCartridgeDlgKey", "Choose Cartridge", ".65c", ".");
        }
#endif
        }
    }

#ifdef TARGET_WASM
    ImGui::SameLine();
    if (ImGui::Button("SDK")) {
        if (!state.sdk.loaded) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                try {
                    auto j = nlohmann::json::parse(std::string(reinterpret_cast<const char*>(data), size));
                    state.sdk.roms = j.at("roms").get<std::vector<std::string>>();
                    state.sdk.midis = j.at("midis").get<std::vector<std::string>>();
                    state.sdk.vrams = j.at("vrams").get<std::vector<std::string>>();
                    state.sdk.loaded = true;
                    state.sdk.showPopup = true;
                } catch (...) {
                    printf("Failed to parse roms.json\n");
                }
            };
            WebFileUtils::fetch_file("roms/roms.json", "roms.json");
        } else {
            state.sdk.showPopup = true;
        }
    }
    DrawSDKPopup(state);
#endif

    ImGui::SameLine();
    if (ImGui::Button("Copy Output")) {
        ImGui::OpenPopup("CopyConsoleOutput");
    }

#ifndef TARGET_WASM
    ImGui::SameLine();
    ImGui::BeginDisabled(cartLoaded);
    if (ImGui::Button("Open IDE")) {
        state.ide.open = !state.ide.open;
        if (state.ide.code.empty()) {
            state.ide.code = "// Write your C or Assembly here!\n";
        }
    }
    ImGui::EndDisabled();
    if (cartLoaded && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Eject cartridge first to use the IDE");
            ImGui::EndTooltip();
        }
    }
#endif
    ImGui::Separator();
}

static void HandleQueueCharacters(AppState& state, const ImGuiIO& imgui_io) {
    for (int idx = 0; idx < imgui_io.InputQueueCharacters.Size; idx++) {
        unsigned int chr = imgui_io.InputQueueCharacters[idx];
        if (chr > 0 && chr < 0x80) {
            if (chr == '\r' || chr == '\n') {
                continue;
            }
            state.emulator.InjectKey((char)chr);
        }
    }
}

static void HandleSpecialKeys(AppState& state) {
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
}

static void HandleCtrlKeys(AppState& state, const ImGuiIO& imgui_io) {
    if (imgui_io.KeyCtrl) {
        for (int key = ImGuiKey_A; key <= ImGuiKey_Z; key++) {
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
                state.emulator.InjectKey(static_cast<char>(1 + (key - static_cast<int>(ImGuiKey_A))));
            }
        }
    }
}

static void HandleConsoleInput(AppState& state) {
    const ImGuiIO& imgui_io = ImGui::GetIO();
    if (ImGui::IsWindowFocused()) {
        SDL_Window* sdl_window = SDL_GetKeyboardFocus();
        if (sdl_window != nullptr) {
            SDL_StartTextInput(sdl_window);
        }

        HandleQueueCharacters(state, imgui_io);
        HandleSpecialKeys(state);
        HandleCtrlKeys(state, imgui_io);
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
    ImGui::SetNextWindowSize(ImVec2(state.emulation.gpuEnabled ? left_width : work_size.x, bottom_section_height),
                             ImGuiCond_Always);
    ImGui::Begin("Console", nullptr, window_flags);

    DrawConsoleButtonBar(state);
    HandleConsoleInput(state);
    DrawConsoleText();
    DrawCopyOutputModal();

    ImGui::End();
}

}  // namespace GUI
