#include "Frontend/GUI/ControlWindow.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <chrono>   // NOLINT
#include <cstdint>  // NOLINT
#include <ctime>    // NOLINT
#include <fstream>
#include <iomanip>  // NOLINT
#include <sstream>
#include <string_view>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"
#include "Hardware/Comm/SDCard.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static bool IsSDCardEnabled(AppState& state) {
    auto& bus = state.emulator.GetMem();
    const auto& devices = bus.GetRegisteredDevices();
    for (const auto& reg : devices) {
        if (reg.device == static_cast<IBusDevice*>(&state.emulator.GetSDCard())) {
            return reg.enabled;
        }
    }
    return false;
}

// Creates a blank FAT16 disk image (32 MB) in pure C++ — no host tools required.
static bool CreateFAT16Image(const std::string& path) {
    // Geometry for a 32 MB image
    static constexpr uint32_t BYTES_PER_SECTOR = 512;
    static constexpr uint32_t SECTORS_PER_CLUSTER = 8;
    static constexpr uint32_t RESERVED_SECTORS = 4;
    static constexpr uint32_t NUM_FATS = 2;
    static constexpr uint32_t ROOT_ENTRIES = 512;
    static constexpr uint32_t TOTAL_SECTORS = 65536;  // 32 MB
    static constexpr uint32_t FAT_SECTORS = 32;       // ceil(8192 clusters * 2 bytes / 512)
    static constexpr uint32_t ROOT_SECTORS = (ROOT_ENTRIES * 32) / BYTES_PER_SECTOR;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }

    // Pre-fill with zeros
    std::string zero(BYTES_PER_SECTOR, '\0');
    for (uint32_t i = 0; i < TOTAL_SECTORS; i++) {
        file.write(zero.data(), BYTES_PER_SECTOR);
    }
    file.seekp(0, std::ios::beg);

    // --- Boot sector (BPB) ---
    std::array<char, BYTES_PER_SECTOR> boot{};
    // Jump + NOP
    boot[0] = static_cast<char>(0xEB);
    boot[1] = static_cast<char>(0x3C);
    boot[2] = static_cast<char>(0x90);
    // OEM Name
    std::string_view oem = "MSDOS5.0";
    for (size_t i = 0; i < oem.length(); i++) {
        boot.at(3 + i) = static_cast<char>(oem.at(i));
    }
    // Bytes per sector (512)
    boot[11] = static_cast<char>(0x00);
    boot[12] = static_cast<char>(0x02);
    // Sectors per cluster
    boot[13] = static_cast<char>(SECTORS_PER_CLUSTER);
    // Reserved sectors
    boot[14] = static_cast<char>(RESERVED_SECTORS & 0xFF);
    boot[15] = static_cast<char>(RESERVED_SECTORS >> 8);
    // Number of FATs
    boot[16] = static_cast<char>(NUM_FATS);
    // Root entry count
    boot[17] = static_cast<char>(ROOT_ENTRIES & 0xFF);
    boot[18] = static_cast<char>(ROOT_ENTRIES >> 8);
    // Total sectors 16
    boot[19] = static_cast<char>(0x00);
    boot[20] = static_cast<char>(0x00);  // use 32-bit field
    // Media type (fixed disk)
    boot[21] = static_cast<char>(0xF8);
    // FAT size in sectors
    boot[22] = static_cast<char>(FAT_SECTORS & 0xFF);
    boot[23] = static_cast<char>(FAT_SECTORS >> 8);
    // Sectors per track / heads (63 / 255)
    boot[24] = static_cast<char>(63);
    boot[25] = static_cast<char>(0);
    boot[26] = static_cast<char>(255);
    boot[27] = static_cast<char>(0);
    // Hidden sectors
    boot[28] = boot[29] = boot[30] = boot[31] = static_cast<char>(0);
    // Total sectors 32
    boot[32] = static_cast<char>(TOTAL_SECTORS & 0xFF);
    boot[33] = static_cast<char>((TOTAL_SECTORS >> 8) & 0xFF);
    boot[34] = static_cast<char>((TOTAL_SECTORS >> 16) & 0xFF);
    boot[35] = static_cast<char>((TOTAL_SECTORS >> 24) & 0xFF);
    // Drive number, reserved, boot sig
    boot[36] = static_cast<char>(0x80);
    boot[37] = static_cast<char>(0x00);
    boot[38] = static_cast<char>(0x29);
    // Volume ID (arbitrary)
    boot[39] = static_cast<char>(0xDE);
    boot[40] = static_cast<char>(0xAD);
    boot[41] = static_cast<char>(0xBE);
    boot[42] = static_cast<char>(0xEF);
    // Volume label
    std::string_view label = "SD IMAGE   ";
    for (size_t i = 0; i < label.length(); i++) {
        boot.at(43 + i) = static_cast<char>(label.at(i));
    }
    // FS type string
    std::string_view fstype = "FAT16   ";
    for (size_t i = 0; i < fstype.length(); i++) {
        boot.at(54 + i) = static_cast<char>(fstype.at(i));
    }
    // Boot signature
    boot[510] = static_cast<char>(0x55);
    boot[511] = static_cast<char>(0xAA);
    file.write(boot.data(), BYTES_PER_SECTOR);

    // --- FAT sectors (two copies) ---
    // First two entries are reserved: 0xFFF8 (media) and 0xFFFF
    std::array<char, BYTES_PER_SECTOR> fatSector{};
    fatSector[0] = static_cast<char>(0xF8);
    fatSector[1] = static_cast<char>(0xFF);  // FAT entry 0 (media)
    fatSector[2] = static_cast<char>(0xFF);
    fatSector[3] = static_cast<char>(0xFF);  // FAT entry 1

    // Seek to first FAT
    file.seekp(static_cast<std::streamoff>(RESERVED_SECTORS) * BYTES_PER_SECTOR, std::ios::beg);
    file.write(fatSector.data(), BYTES_PER_SECTOR);
    // Remaining FAT sectors already zero
    std::array<char, BYTES_PER_SECTOR> zeroBuf{};
    for (uint32_t size = 1; size < FAT_SECTORS; size++) {
        file.write(zeroBuf.data(), BYTES_PER_SECTOR);
    }

    // Second FAT copy
    file.write(fatSector.data(), BYTES_PER_SECTOR);
    for (uint32_t size = 1; size < FAT_SECTORS; size++) {
        file.write(zeroBuf.data(), BYTES_PER_SECTOR);
    }

    // Root directory is already zero-filled from the pre-fill step.
    (void)ROOT_SECTORS;

    return file.good();
}

static void HandleReset(AppState& state) {
    bool wasRunning = !state.emulator.IsPaused();
    if (wasRunning) {
        state.emulator.Pause();
    }

    Console::Clear();
    state.emulator.GetGPU().Reset();
    state.emulator.ClearProfiler();

    if (state.emulator.GetCartridge().loaded) {
        // Reset cartridge state (full hardware re-init from manifest)
        state.emulator.SetupHardware();
    } else if (state.rom.loaded) {
        std::string errorMsg;
#ifdef TARGET_WASM
        if (!state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), state.rom.bin, errorMsg)) {
#else
        bool initSuccess = false;
        if (!state.rom.data.empty()) {
            initSuccess = state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), state.rom.bin, errorMsg);
        } else {
            initSuccess = state.emulator.Init(state.rom.bin, errorMsg);
        }
        if (!initSuccess) {
#endif
            std::cerr << "Error resetting ROM: " << errorMsg << '\n';
            state.rom.loaded = false;
#ifndef TARGET_WASM
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
#endif
        } else {
            state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
        }
    }

    if (wasRunning) {
        state.emulator.Resume();
    }
}

static void DrawPlaybackControls(AppState& state) {
    ImGui::BeginDisabled((!state.rom.loaded && !state.script.loaded) || state.emulator.IsHalted());
    if (ImGui::Button(state.emulator.IsPaused() ? "Run" : "Pause")) {
        if (state.emulator.IsPaused()) {
            state.emulator.Resume();
        } else {
            state.emulator.Pause();
        }
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.emulator.IsPaused() || state.emulator.IsHalted());
    if (ImGui::Button("Step")) {
        state.emulator.Step();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.emulator.CanRewind() || !state.emulator.IsRunning() || !state.emulator.IsPaused());
    if (ImGui::Button("Rewind")) {
        state.emulator.Rewind();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(
            "Rewind to the previous state (max 255 states).\nOnly available when the emulator is paused.");
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();
}

static void DrawAudioVideoControls(AppState& state) {
    bool gpuOverridden = state.emulator.GetCartridge().config.gpuEnabled.has_value();
    ImGui::BeginDisabled(gpuOverridden);
    if (ImGui::Checkbox("GPU", &state.emulation.gpuEnabled)) {
        state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
    }
    ImGui::EndDisabled();
    if (gpuOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Managed by Cartridge");
            ImGui::EndTooltip();
        }
    }

    ImGui::SameLine();
    bool soundEnabled = state.emulator.GetSID().IsSoundEnabled();
    bool sidOverridden = state.emulator.GetCartridge().config.sidEnabled.has_value();
    ImGui::BeginDisabled(sidOverridden);
    if (ImGui::Checkbox("SID", &soundEnabled)) {
        state.emulator.GetSID().EnableSound(soundEnabled);
    }
    ImGui::EndDisabled();
    if (sidOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Managed by Cartridge");
            ImGui::EndTooltip();
        }
    }
}

static void DrawControlButtonBar(AppState& state) {
    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup("SettingsMenu");
    }
    ImGui::SameLine();
    if (ImGui::Button("Debugger")) {
        state.debugger.open = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        HandleReset(state);
    }

    ImGui::SameLine();
    DrawPlaybackControls(state);

    ImGui::SameLine();
    DrawAudioVideoControls(state);
}

static void DrawSettingsBasic(AppState& state) {
    bool cycleOverridden = state.emulator.GetCartridge().config.cycleAccurate.has_value();
    ImGui::BeginDisabled(cycleOverridden);
    if (ImGui::Checkbox("Cycle-Accurate", &state.emulation.cycleAccurate)) {
        state.emulator.SetCycleAccurate(state.emulation.cycleAccurate);
    }
    ImGui::EndDisabled();
    if (cycleOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Managed by Cartridge");
            ImGui::EndTooltip();
        }
    }

    ImGui::Checkbox("Force load savestate", &state.emulation.forceLoadSaveState);
#ifdef TARGET_WASM
    bool dummyAutoReload = false;
    ImGui::BeginDisabled(true);
    ImGui::Checkbox("Auto-Reload Bin", &dummyAutoReload);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Auto-reload is not supported in the web build.");
            ImGui::EndTooltip();
        }
    }
#else
    if (ImGui::Checkbox("Auto-Reload Bin", &state.emulation.autoReload)) {
        state.emulator.SetAutoReload(state.emulation.autoReload);
    }
#endif

    ImGui::Separator();
    ImGui::TextUnformatted("Scripting");

    if (state.script.loaded) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("Script Loaded & Running");
        ImGui::PopStyleColor();
        if (ImGui::Button("Stop Script")) {
            state.emulator.GetScriptEngine().Stop();
            state.script.loaded = false;
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("No Script Loaded");
        ImGui::PopStyleColor();
#ifdef TARGET_WASM
        ImGui::BeginDisabled();
        if (ImGui::Button("Load & Run Script (.py)")) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                std::string virtualPath = "/tmp/" + std::string(filename);
                FILE* f = fopen(virtualPath.c_str(), "wb");
                if (f) {
                    fwrite(data, 1, size, f);
                    fclose(f);
                }

                state.script.path = virtualPath;
                state.script.loaded = true;
                state.script.showConsole = true;
                state.emulator.Pause();
                state.emulator.GetScriptEngine().LoadAndRun(virtualPath);
            };
            WebFileUtils::open_browser_file_picker(".py");
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextUnformatted("Python scripting is not supported in the web build.");
                ImGui::EndTooltip();
            }
        }
#else
        if (ImGui::Button("Load & Run Script (.py)")) {
            ImGuiFileDialog::Instance()->OpenDialog("LoadScriptDlgKey", "Load Python Script", ".py", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
#endif
    }

    ImGui::Separator();
    ImGui::TextUnformatted("SD Card Emulation");

    bool cartridgeHasSD = !state.emulator.GetCartridge().sdCardPath.empty();

    bool sdMounted = state.emulator.GetSDCard().IsMounted();
    if (sdMounted) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("SD Card Mounted");
        ImGui::PopStyleColor();
#ifdef TARGET_WASM
        if (ImGui::Button("Save Changes (Download)")) {
            std::string path = state.emulator.GetSDCard().GetMountedPath();
            if (!path.empty()) {
                std::ifstream ifs(path, std::ios::binary);
                if (ifs.is_open()) {
                    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                    std::string filename = std::filesystem::path(path).filename().string();
                    WebFileUtils::download_file(filename.c_str(), buffer.data(), buffer.size());
                }
            }
        }
        ImGui::SameLine();
#endif
        if (ImGui::Button(
#ifdef TARGET_WASM
            "Unmount & Save (Download)"
#else
            "Unmount SD Card"
#endif
        )) {
#ifdef TARGET_WASM
            std::string path = state.emulator.GetSDCard().GetMountedPath();
            if (!path.empty()) {
                std::ifstream ifs(path, std::ios::binary);
                if (ifs.is_open()) {
                    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                    std::string filename = std::filesystem::path(path).filename().string();
                    WebFileUtils::download_file(filename.c_str(), buffer.data(), buffer.size());
                }
            }
#endif
            state.emulator.GetSDCard().Unmount();
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("SD Card Not Mounted");
        ImGui::PopStyleColor();
#ifdef TARGET_WASM
        if (ImGui::Button("Create New SD Image...")) {
            // Create a default image and download it
            std::string tempPath = "/tmp/new_sd.img";
            if (CreateFAT16Image(tempPath)) {
                std::ifstream ifs(tempPath, std::ios::binary);
                std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                WebFileUtils::download_file("new_sd.img", buffer.data(), buffer.size());
                
                if (IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(tempPath);
                    state.popups.sdCardWebWarning = true;
                }
            }
        }
        if (ImGui::Button("Mount Image (IMG)")) {
            WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
                std::string virtualPath = "/tmp/" + std::string(filename);
                FILE* f = fopen(virtualPath.c_str(), "wb");
                if (f) {
                    fwrite(data, 1, size, f);
                    fclose(f);
                }

                if (IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(virtualPath);
                    state.popups.sdCardWebWarning = true;
                } else {
                    state.popups.sdCardDisabled = true;
                }
            };
            WebFileUtils::open_browser_file_picker(".img");
        }
        if (cartridgeHasSD && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetItemTooltip("%s", "SD Card is managed by the active cartridge.");
        }
        ImGui::EndDisabled();
#else
        ImGui::BeginDisabled(cartridgeHasSD);
        if (ImGui::Button("Create New SD Image...")) {
            ImGuiFileDialog::Instance()->OpenDialog("CreateSDDlgKey", "Save New SD Image", ".img", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
        if (ImGui::Button("Mount Image (IMG)")) {
            ImGuiFileDialog::Instance()->OpenDialog("MountSDDlgKey", "Mount SD Image", ".img", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
        if (cartridgeHasSD && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetItemTooltip("%s", "SD Card is managed by the active cartridge.");
        }
        ImGui::EndDisabled();
#endif
    }
}

static void DrawSettingsSaveState(AppState& state) {
#ifdef TARGET_WASM
    if (ImGui::Button("Save State")) {
        std::string tempPath = "/tmp/save.savestate";
        if (state.emulator.SaveState(tempPath)) {
            std::ifstream ifs(tempPath, std::ios::binary);
            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            WebFileUtils::download_file("emulator_state.savestate", buffer.data(), buffer.size());
        }
    }
    if (ImGui::Button("Load State")) {
        WebFileUtils::onFilePickedCallback = [&state](const char* filename, const uint8_t* data, int size) {
            std::string virtualPath = "/tmp/" + std::string(filename);
            FILE* f = fopen(virtualPath.c_str(), "wb");
            if (f) {
                fwrite(data, 1, size, f);
                fclose(f);
            }

            state.emulator.Pause();
            state.emulator.LoadState(virtualPath, state.emulation.forceLoadSaveState);
            state.rom.loaded = true;
            state.emulation.gpuEnabled = state.emulator.IsGPUEnabled();
            state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
            state.emulation.cycleAccurate = state.emulator.IsCycleAccurate();
            state.emulation.autoReload = state.emulator.IsAutoReloadEnabled();
            state.rom.bin = state.emulator.GetCurrentBinPath();
        };
        WebFileUtils::open_browser_file_picker(".savestate");
    }
#else
    if (ImGui::Button("Save State")) {
        // Build default filename: SIM65C02SST_<bin>_<date>.savestate
        std::string binName = "unknown";
        if (!state.rom.bin.empty()) {
            binName = std::filesystem::path(state.rom.bin).stem().string();
        }
        auto now = std::chrono::system_clock::now();
        std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
        std::tm* timeStruct = std::localtime(&timeValue);
        std::ostringstream dateStr;
        dateStr << (timeStruct->tm_year + 1900) << (timeStruct->tm_mon + 1 < 10 ? "0" : "") << (timeStruct->tm_mon + 1)
                << (timeStruct->tm_mday < 10 ? "0" : "") << timeStruct->tm_mday << "_"
                << (timeStruct->tm_hour < 10 ? "0" : "") << timeStruct->tm_hour << (timeStruct->tm_min < 10 ? "0" : "")
                << timeStruct->tm_min << (timeStruct->tm_sec < 10 ? "0" : "") << timeStruct->tm_sec;
        std::string dateOutput = dateStr.str();
        std::string defaultName = "SIM65C02SST_" + binName + "_" + dateOutput;
        ImGuiFileDialog::Instance()->OpenDialog("SaveStateDlgKey", "Save State", ".savestate", ".", defaultName);
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Load State")) {
        ImGuiFileDialog::Instance()->OpenDialog("LoadStateDlgKey", "Load State", ".savestate", ".");
        ImGui::CloseCurrentPopup();
    }
#endif
}

static void DrawSettingsCRT(AppState& state) {
    ImGui::TextUnformatted("CRT Filters (GPU)");
    auto setCRTAll = [&](bool val) {
        state.crt.scanlines = state.crt.interlacing = state.crt.curvature = state.crt.chromatic = state.crt.blur =
            state.crt.shadowMask = state.crt.vignette = state.crt.cornerRounding = state.crt.glassGlare =
                state.crt.colorBleeding = state.crt.noise = state.crt.vsyncJitter = state.crt.phosphorDecay =
                    state.crt.bloom = state.crt.ghosting = state.crt.halation = state.crt.moire = val;
        if (!val) {
            state.crt.gamma = 2.75F;
        }
    };
    if (ImGui::Button("All On")) {
        setCRTAll(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("All Off")) {
        setCRTAll(false);
    }
    ImGui::TextUnformatted("Essentials");
    ImGui::Checkbox("Scanlines", &state.crt.scanlines);
    ImGui::Checkbox("Interlacing", &state.crt.interlacing);
    ImGui::Checkbox("Screen Curvature", &state.crt.curvature);
    ImGui::Checkbox("Chromatic Aberration", &state.crt.chromatic);
    ImGui::Checkbox("Phosphor Blur", &state.crt.blur);
    ImGui::TextUnformatted("Screen Physicality");
    ImGui::Checkbox("Shadow Mask", &state.crt.shadowMask);
    ImGui::Checkbox("Vignette", &state.crt.vignette);
    ImGui::Checkbox("Corner Rounding", &state.crt.cornerRounding);
    ImGui::Checkbox("Glass Glare", &state.crt.glassGlare);
    ImGui::TextUnformatted("Signal & Analog");
    ImGui::Checkbox("Color Bleeding", &state.crt.colorBleeding);
    ImGui::Checkbox("RF Noise", &state.crt.noise);
    ImGui::Checkbox("VSync Jitter", &state.crt.vsyncJitter);
    ImGui::Checkbox("Phosphor Decay", &state.crt.phosphorDecay);
    ImGui::TextUnformatted("Lighting");
    ImGui::Checkbox("Bloom", &state.crt.bloom);
    ImGui::Checkbox("Halation", &state.crt.halation);
    ImGui::TextUnformatted("Signal Advanced");
    ImGui::Checkbox("Ghosting (Echo)", &state.crt.ghosting);
    ImGui::Checkbox("Moiré Pattern", &state.crt.moire);
    ImGui::SliderFloat("Gamma", &state.crt.gamma, 1.0F, 3.5F, "%.2f");
}

static void DrawSettingsPopup(AppState& state) {
    if (ImGui::BeginPopup("SettingsMenu")) {
        DrawSettingsBasic(state);
        ImGui::Separator();
        DrawSettingsSaveState(state);
        ImGui::Separator();
        DrawSettingsCRT(state);

        ImGui::EndPopup();
    }
}

static void DrawTargetIPS(AppState& state, float mainColWidth) {
    int tempIPS = state.emulation.instructionsPerFrame;
    std::string targetLabel = "Target ";
    targetLabel += (state.emulation.cycleAccurate ? "Hz" : "IPS");

    bool ipsOverridden = state.emulator.GetCartridge().config.targetIPS.has_value();
    ImGui::BeginDisabled(ipsOverridden);
    ImGui::SetNextItemWidth(mainColWidth * 0.3F);
    if (ImGui::InputInt(targetLabel.c_str(), &tempIPS, 1000000, 1000000)) {
        if (state.emulation.instructionsPerFrame == 1 && tempIPS == 1000001) {
            tempIPS = 1000000;
        }

        tempIPS = std::max(1, tempIPS);
        state.emulation.instructionsPerFrame = tempIPS;
        state.emulator.SetTargetIPS(state.emulation.instructionsPerFrame);
    }
    ImGui::EndDisabled();
    if (ipsOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Managed by Cartridge");
            ImGui::EndTooltip();
        }
    }
}

static void DrawActualIPS(AppState& state) {
    int actualIPS = state.emulator.GetActualIPS();
    std::ostringstream oss;
    if (state.emulation.cycleAccurate) {
        if (actualIPS >= 1000000) {
            oss << "Sim: " << std::fixed << std::setprecision(2) << (static_cast<float>(actualIPS) / 1000000.0F) << " MHz";
        } else if (actualIPS >= 1000) {
            oss << "Sim: " << std::fixed << std::setprecision(2) << (static_cast<float>(actualIPS) / 1000.0F) << " KHz";
        } else {
            oss << "Sim: " << actualIPS << " Hz";
        }
    } else {
        if (actualIPS >= 1000000) {
            oss << "Sim: " << std::fixed << std::setprecision(2) << (static_cast<float>(actualIPS) / 1000000.0F) << " MIPS";
        } else if (actualIPS >= 1000) {
            oss << "Sim: " << std::fixed << std::setprecision(2) << (static_cast<float>(actualIPS) / 1000.0F) << " KIPS";
        } else {
            oss << "Sim: " << actualIPS << " IPS";
        }
    }
    ImGui::TextUnformatted(oss.str().c_str());
}
static void DrawVideoOptions(AppState& state) {
    ImGui::Separator();
    ImGui::TextUnformatted("Video Tracks");
    ImGui::Checkbox("Raw Track", &state.emulation.recordingSettings.recordRaw);
    ImGui::Checkbox("Processed Track", &state.emulation.recordingSettings.recordProcessed);

    // Forcing MKV if both tracks are selected
    bool forcedMKV = state.emulation.recordingSettings.recordRaw && state.emulation.recordingSettings.recordProcessed;
    if (forcedMKV) {
        state.emulation.recordingSettings.format = VideoFormat::MKV;
        ImGui::BeginDisabled(true);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Format");
    int format = static_cast<int>(state.emulation.recordingSettings.format);
    if (ImGui::RadioButton("MKV", format == 0)) {
        state.emulation.recordingSettings.format = VideoFormat::MKV;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MP4", format == 1)) {
        state.emulation.recordingSettings.format = VideoFormat::MP4;
    }

    if (forcedMKV) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Dual track recording requires MKV format.");
            ImGui::EndTooltip();
        }
    }
}

static void HandleSIDWindowSettings(AppState& state) {
    // Force MP4 and Processed track for SID Window
    state.emulation.recordingSettings.format = VideoFormat::MP4;
    state.emulation.recordingSettings.recordRaw = false;
    state.emulation.recordingSettings.recordProcessed = true;
}

static void DrawStartRecordingButton(AppState& state) {
    bool canStart = true;
    if (state.emulation.recordingSettings.type == RecordingType::Video) {
        canStart = (state.emulation.recordingSettings.recordRaw || state.emulation.recordingSettings.recordProcessed);
    }

    if (!canStart) {
        ImGui::BeginDisabled(true);
    }

    ImGui::Separator();
    if (ImGui::Button("Start Recording", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        if (state.emulation.recordingSettings.type == RecordingType::Audio) {
            const char* ext = (state.emulation.recordingSettings.audioFormat == Control::AudioFormat::OPUS) ? ".ogg" : ".flac";
            ImGuiFileDialog::Instance()->OpenDialog("RecordSIDDlgKey", "Save Audio Recording", ext, ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        } else {
            std::string ext = (state.emulation.recordingSettings.format == VideoFormat::MP4) ? ".mp4" : ".mkv";
            ImGuiFileDialog::Instance()->OpenDialog("RecordVideoDlgKey", "Save Video Recording", ext.c_str(), ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
        ImGui::CloseCurrentPopup();
    }

    if (!canStart) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Select at least one video track to record.");
            ImGui::EndTooltip();
        }
    }
}

static void DrawRecordingConfigPopup(AppState& state) {
    if (ImGui::BeginPopup("RecordingConfig", ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Recording Mode");

        int type = static_cast<int>(state.emulation.recordingSettings.type);
        if (ImGui::RadioButton("Audio", type == 0)) {
            state.emulation.recordingSettings.type = RecordingType::Audio;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Video", type == 1)) {
            state.emulation.recordingSettings.type = RecordingType::Video;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("SID Window", type == 2)) {
            state.emulation.recordingSettings.type = RecordingType::SIDWindow;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Audio Format");
        int aFormat = static_cast<int>(state.emulation.recordingSettings.audioFormat);
        if (ImGui::RadioButton("FLAC", aFormat == 0)) {
            state.emulation.recordingSettings.audioFormat = Control::AudioFormat::FLAC;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("OPUS", aFormat == 1)) {
            state.emulation.recordingSettings.audioFormat = Control::AudioFormat::OPUS;
        }

        if (state.emulation.recordingSettings.type == RecordingType::Video) {
            DrawVideoOptions(state);
        } else if (state.emulation.recordingSettings.type == RecordingType::SIDWindow) {
            HandleSIDWindowSettings(state);
        }

        DrawStartRecordingButton(state);
        ImGui::EndPopup();
    }
}

static void DrawRecordingControls(AppState& state) {
    bool isRecording = state.emulation.isRecordingVideo || state.emulator.GetSID().IsRecording();

    ImGui::BeginDisabled(!state.rom.loaded);

    if (isRecording) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8F, 0.0F, 0.0F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0F, 0.2F, 0.2F, 1.0F));
    }

#ifdef TARGET_WASM
    ImGui::BeginDisabled(true);
#endif
    if (ImGui::Button(isRecording ? "STOP REC" : "REC")) {
        if (isRecording) {
            if (state.emulation.isRecordingVideo) {
                state.emulation.isRecordingVideo = false;
            } else {
                state.emulator.GetSID().StopRecording();
            }
        } else {
            ImGui::OpenPopup("RecordingConfig");
        }
    }
#ifdef TARGET_WASM
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Recording is only supported in local builds.");
    }
#endif

    if (isRecording) {
        ImGui::PopStyleColor(2);
    }

    DrawRecordingConfigPopup(state);

    ImGui::EndDisabled();
}

static void DrawIPSSection(AppState& state, float mainColWidth) {
    DrawTargetIPS(state, mainColWidth);

    ImGui::SameLine();
    ImGui::TextUnformatted(" || ");

    ImGui::SameLine();
    DrawActualIPS(state);

    ImGui::SameLine();
    DrawRecordingControls(state);
}

static void HandleCreateSDDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("CreateSDDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            // Ensure .img extension
            if (filePath.size() < 4 || filePath.substr(filePath.size() - 4) != ".img") {
                filePath += ".img";
            }
            if (CreateFAT16Image(filePath)) {
                if (IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(filePath);
                } else {
                    state.popups.sdCardDisabled = true;
                }
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleMountSDDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("MountSDDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            if (IsSDCardEnabled(state)) {
                state.emulator.GetSDCard().Mount(filePath);
            } else {
                state.popups.sdCardDisabled = true;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleLoadScriptDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("LoadScriptDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            state.script.path = filePath;
            state.script.loaded = true;
            state.script.showConsole = true;
            state.emulator.Pause();  // Pause the frontend before running the script
            state.emulator.GetScriptEngine().LoadAndRun(filePath);
            ImGui::CloseCurrentPopup();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleRecordAudioDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("RecordSIDDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.GetSID().StartRecording(filePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleRecordVideoDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("RecordVideoDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulation.recordingVideoPath = filePath;
            state.emulation.isRecordingVideo = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleDialogs(AppState& state) {
    HandleCreateSDDialog(state);
    HandleMountSDDialog(state);
    HandleLoadScriptDialog(state);
    HandleRecordAudioDialog(state);
    HandleRecordVideoDialog(state);
}

void DrawControlWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47F;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, top_section_height * 0.4F), ImGuiCond_Always);
    ImGui::Begin("Control", nullptr, window_flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    DrawControlButtonBar(state);
    DrawSettingsPopup(state);
    DrawDebugMenu(state);
    DrawIPSSection(state, mainColWidth);

    HandleDialogs(state);

    ImGui::SetScrollHereY(1.0F);
    ImGui::End();
}

}  // namespace GUI
