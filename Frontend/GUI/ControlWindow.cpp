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

    if (state.rom.loaded) {
        std::string errorMsg;
        if (!state.emulator.Init(state.rom.bin, errorMsg)) {
            std::cerr << "Error resetting ROM: " << errorMsg << '\n';
            state.rom.loaded = false;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
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
    if (ImGui::Checkbox("GPU", &state.emulation.gpuEnabled)) {
        state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
    }
    ImGui::SameLine();
    bool soundEnabled = state.emulator.GetSID().IsSoundEnabled();
    if (ImGui::Checkbox("SID", &soundEnabled)) {
        state.emulator.GetSID().EnableSound(soundEnabled);
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
    if (ImGui::Checkbox("Cycle-Accurate", &state.emulation.cycleAccurate)) {
        state.emulator.SetCycleAccurate(state.emulation.cycleAccurate);
    }
    ImGui::Checkbox("Force load savestate", &state.emulation.forceLoadSaveState);
    if (ImGui::Checkbox("Auto-Reload Bin", &state.emulation.autoReload)) {
        state.emulator.SetAutoReload(state.emulation.autoReload);
    }

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
        if (ImGui::Button("Load & Run Script (.py)")) {
            ImGuiFileDialog::Instance()->OpenDialog("LoadScriptDlgKey", "Load Python Script", ".py", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("SD Card Emulation");

    // We only show SD Card state, but user must explicitly mount via file dialog.
    bool sdMounted = state.emulator.GetSDCard().IsMounted();
    if (sdMounted) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 1.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("SD Card Mounted");
        ImGui::PopStyleColor();
        if (ImGui::Button("Unmount SD Card")) {
            state.emulator.GetSDCard().Unmount();
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::TextUnformatted("SD Card Not Mounted");
        ImGui::PopStyleColor();
        if (ImGui::Button("Create New SD Image...")) {
            ImGuiFileDialog::Instance()->OpenDialog("CreateSDDlgKey", "Save New SD Image", ".img", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
        if (ImGui::Button("Mount Image (IMG)")) {
            ImGuiFileDialog::Instance()->OpenDialog("MountSDDlgKey", "Mount SD Image", ".img", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_None);
        }
    }
}

static void DrawSettingsSaveState(AppState& state) {
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
}

static void DrawSettingsCRT(AppState& state) {
    ImGui::TextUnformatted("CRT Filters (GPU)");
    auto setCRTAll = [&](bool val) {
        state.crt.scanlines = state.crt.interlacing = state.crt.curvature = state.crt.chromatic = state.crt.blur =
            state.crt.shadowMask = state.crt.vignette = state.crt.cornerRounding = state.crt.glassGlare =
                state.crt.colorBleeding = state.crt.noise = state.crt.vsyncJitter = state.crt.phosphorDecay =
                    state.crt.bloom = val;
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

    ImGui::SetNextItemWidth(mainColWidth * 0.3F);
    if (ImGui::InputInt(targetLabel.c_str(), &tempIPS, 1000000, 1000000)) {
        if (state.emulation.instructionsPerFrame == 1 && tempIPS == 1000001) {
            tempIPS = 1000000;
        }

        tempIPS = std::max(1, tempIPS);
        state.emulation.instructionsPerFrame = tempIPS;
        state.emulator.SetTargetIPS(state.emulation.instructionsPerFrame);
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

static void DrawAudioRecordingButton(AppState& state, Core::SID& sid) {
    bool isRecordingAudio = sid.IsRecording();
    if (isRecordingAudio) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0F, 0.2F, 0.2F, 1.0F));
    }
    if (ImGui::Button(isRecordingAudio ? "Stop Audio REC" : "REC Audio")) {
        if (isRecordingAudio) {
            sid.StopRecording();
        } else {
            ImGuiFileDialog::Instance()->OpenDialog("RecordSIDDlgKey", "Save Audio Recording", ".flac", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
    }
    if (isRecordingAudio) {
        ImGui::PopStyleColor(2);
    }
}

static void DrawVideoRecordingButton(AppState& state) {
    bool isRecordingVideo = state.emulation.isRecordingVideo;
    if (isRecordingVideo) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0F, 0.2F, 0.2F, 1.0F));
    }

    if (ImGui::Button(isRecordingVideo ? "Stop Video REC" : "REC Video")) {
        if (isRecordingVideo) {
            state.emulation.isRecordingVideo = false;
        } else {
            ImGuiFileDialog::Instance()->OpenDialog("RecordVideoDlgKey", "Save Video Recording", ".mkv", ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_ConfirmOverwrite);
        }
    }
    if (isRecordingVideo) {
        ImGui::PopStyleColor(2);
    }
}

static void DrawRecordingControls(AppState& state) {
    auto& sid = state.emulator.GetSID();
    bool isRecordingAudio = sid.IsRecording();
    bool isRecordingVideo = state.emulation.isRecordingVideo;

    ImGui::BeginDisabled(!state.rom.loaded);

    // Display Audio REC button only if NOT recording video
    if (!isRecordingVideo) {
        DrawAudioRecordingButton(state, sid);
        ImGui::SameLine();
    }

    // Display Video REC button only if NOT recording audio
    if (!isRecordingAudio) {
        DrawVideoRecordingButton(state);
    }

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

static void HandleRecordSIDDialog(AppState& state) {
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
    HandleRecordSIDDialog(state);
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
