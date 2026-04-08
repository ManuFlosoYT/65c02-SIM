#include "Frontend/GUI/ConsoleWindow.h"
#include <bit>

#include "Frontend/UI/CustomFileDialog.h"
#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "Frontend/Control/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#include <nlohmann/json.hpp>
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Frontend/Control/CartridgeUtils.h"
#else
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/GUI/SDKUpdatePopup.h"
#include "Frontend/Control/SDKManager.h"
#include "Frontend/Control/CartridgeUtils.h"
#endif

namespace GUI {

static void HandleSDKFileLoading(AppState& state, const std::string& filename, const uint8_t* data, int size) {
    if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".65c") {
        state.emulator.Pause();
        std::string errorMsg;
        Core::Cartridge cart;
        if (Core::CartridgeLoader::LoadFromMemory(data, size, cart, errorMsg)) {
            Control::ApplyCartridgeConfig(state, cart);
            if (state.emulator.InitFromMemory(cart.romData.data(), cart.romData.size(), cart.romFileName,
                                              errorMsg)) {
                state.rom.bin = filename;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.ClearProfiler();
            }
        } else {
            std::cerr << "Failed to load cartridge: " << errorMsg << "\n";
        }
    } else {
        std::span<const uint8_t> dataSpan(data, static_cast<size_t>(size));
        state.rom.data.assign(dataSpan.begin(), dataSpan.end());
        state.emulator.Pause();
        std::string errorMsg;
        if (state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), filename,
                                          errorMsg)) {
            Console::Clear();
            state.rom.bin = filename;
            state.rom.loaded = true;
            state.rom.symbols.Clear();
            state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
            state.emulator.ClearProfiler();
        }
    }
}

static void DrawSDKColumn(AppState& state, const std::vector<std::string>& files, const std::string& subfolder) {
    for (const auto& file : files) {
        if (ImGui::Selectable(file.c_str())) {
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
            
            std::string errorMsg;
            Core::Cartridge cart;
            state.emulator.Pause();
            if (Core::CartridgeLoader::Load(path, cart, errorMsg)) {
                Control::ApplyCartridgeConfig(state, cart);
                if (state.emulator.InitFromMemory(cart.romData.data(), cart.romData.size(), cart.romFileName, errorMsg)) {
                    Console::Clear();
                    state.rom.bin = path;
                    state.rom.loaded = true;
                    state.rom.symbols.Clear();
                    state.emulator.ClearProfiler();
                    ImGui::OpenPopup("Cartridge Loaded");
                } else {
                    std::cerr << "SDK: InitFromMemory failed for " << file << ": " << errorMsg << "\n";
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            } else {
                std::cerr << "SDK: Failed to load SDK cartridge " << file << ": " << errorMsg << "\n";
                ImGui::OpenPopup("ErrorLoadingROM");
            }
#endif
            ImGui::CloseCurrentPopup();
        }
    }
}

static void DrawSDKPopup(AppState& state) {
    if (state.sdk.showPopup) {
        ImGui::OpenPopup("SDK ROMs");
        state.sdk.showPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("SDK ROMs", nullptr, ImGuiWindowFlags_None)) {
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
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawLoadROMButton(AppState& state, bool cartLoaded) {
    ImGui::BeginDisabled(cartLoaded);
    if (ImGui::Button("Load ROM")) {
#ifdef TARGET_WASM
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::span<const uint8_t> dataSpan(data, static_cast<size_t>(size));
            state.rom.data.assign(dataSpan.begin(), dataSpan.end());
            state.emulator.Pause();
            std::string errorMsg;
            if (state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), filename, errorMsg)) {
                Console::Clear();
                state.rom.bin = filename;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                state.emulator.ClearProfiler();
                state.emulator.ClearCartridge();
            } else {
                std::cerr << "Failed to load ROM: " << errorMsg << "\n";
            }
        };
        WebFileUtils::open_browser_file_picker(".bin");
#else
        if (!Frontend::CustomFileDialog::IsOpened()) {
            Frontend::CustomFileDialog::OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
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
}

static void DrawLoadCartridgeButton(AppState& state, bool cartLoaded) {
    const char* cartBtnText = cartLoaded ? "Eject Cartridge" : "Load Cartridge (.65c)";
    if (ImGui::Button(cartBtnText)) {
        if (cartLoaded) {
            state.emulator.ClearCartridge();
        } else {
#ifdef TARGET_WASM
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                // Write to virtual FS
                std::string virtualPath = "/tmp/";
                virtualPath += filename;
                FILE* f = fopen(virtualPath.c_str(), "wb");
                if (f) {
                    fwrite(data, 1, static_cast<size_t>(size), f);
                    fclose(f);
                }
            };
            WebFileUtils::open_browser_file_picker(".65c");
#else
            if (!Frontend::CustomFileDialog::IsOpened()) {
                Frontend::CustomFileDialog::OpenDialog("ChooseCartridgeDlgKey", "Choose Cartridge", ".65c", ".");
            }
#endif
        }
    }
}

static void DrawSDKButton(AppState& state) {
    if (ImGui::Button("SDK")) {
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
}

#ifndef TARGET_WASM
static void DrawIDEButton(AppState& state, bool cartLoaded) {
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
}
#endif

static void DrawConsoleButtonBar(AppState& state) {
    bool cartLoaded = state.emulator.GetCartridge().loaded;

    DrawLoadROMButton(state, cartLoaded);

    ImGui::SameLine();
    DrawLoadCartridgeButton(state, cartLoaded);

    ImGui::SameLine();
    DrawSDKButton(state);
    DrawSDKPopup(state);
    DrawSDKUpdatePopup(state);

    ImGui::SameLine();
    if (ImGui::Button("Copy Output")) {
        ImGui::OpenPopup("CopyConsoleOutput");
    }

#ifndef TARGET_WASM
    ImGui::SameLine();
    DrawIDEButton(state, cartLoaded);
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
            state.emulator.InjectKey(static_cast<char>(chr));
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
                fullText += line;
                fullText += "\n";
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
