#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/UI/CustomFileDialog.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include "Frontend/GUI/Style/Style.h"

using Frontend::CustomFileDialog;

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"
#include "Frontend/GUI/SDUtils.h"
#include "Hardware/Core/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/Control/CartridgeUtils.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {



static void DrawPlaybackControls(AppState& state) {
    ImGui::BeginDisabled((!state.rom.loaded && !state.script.loaded) || state.emulator.IsHalted());
    const bool isPaused = state.emulator.IsPaused();
    if (isPaused) {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::RunBg);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::AccentActive);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    }
    if (ImGui::Button(isPaused ? "Run" : "Pause")) {
        if (isPaused) {
            state.emulator.Resume();
        } else {
            state.emulator.Pause();
        }
    }
    if (isPaused) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }
    ImGui::SameLine();
    bool stepDisabled = !state.emulator.IsPaused() || state.emulator.IsHalted();
    ImGui::BeginDisabled(stepDisabled);
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    if (ImGui::Button("Step")) {
        state.emulator.Step();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    if (stepDisabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Emulator must be paused to use Step.");
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.emulator.CanRewind() || !state.emulator.IsRunning() || !state.emulator.IsPaused());
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    if (ImGui::Button("Rewind")) {
        state.emulator.Rewind();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(
            "Rewind to previous frame / instruction state.\nOnly available when the emulator is paused.");
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (gpuOverridden) ImGui::TextUnformatted("Managed by Cartridge");
        else ImGui::TextUnformatted("Enable/Disable Graphics Processing Unit (GPU)");
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    bool soundEnabled = state.emulator.GetSID().IsSoundEnabled();
    bool sidOverridden = state.emulator.GetCartridge().config.sidEnabled.has_value();
    ImGui::BeginDisabled(sidOverridden);
    if (ImGui::Checkbox("SID", &soundEnabled)) {
        state.emulator.GetSID().EnableSound(soundEnabled);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (sidOverridden) ImGui::TextUnformatted("Managed by Cartridge");
        else ImGui::TextUnformatted("Enable/Disable Sound Interface Device (SID)");
        ImGui::EndTooltip();
    }

    if (soundEnabled) {
        ImGui::SameLine();
        bool sidModelOverridden = state.emulator.GetCartridge().config.sidModel.has_value();
        ImGui::BeginDisabled(sidModelOverridden);
        ImGui::PushItemWidth(150);
        const char* sidModels[] = { "MOS 6581", "MOS 8580" };
        int currentSIDModel = (state.emulator.GetSID().GetModel() == Hardware::SIDModel::MOS6581) ? 0 : 1;
        if (ImGui::Combo("##SIDModel", &currentSIDModel, sidModels, 2)) {
            state.emulator.GetSID().SetModel(currentSIDModel == 0 ? Hardware::SIDModel::MOS6581 : Hardware::SIDModel::MOS8580);
        }
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
        if (sidModelOverridden && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextUnformatted("Model forced by Cartridge");
                ImGui::EndTooltip();
            }
        }
    }
}

static void DrawControlButtonBar(AppState& state) {
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
    if (ImGui::Button("Settings")) {
        state.debugger.open = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        Control::PromptOrExecuteCartridgeAction(state, Control::PendingCartridgeAction::Reset);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    DrawPlaybackControls(state);

    ImGui::SameLine();
    DrawAudioVideoControls(state);
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
    const auto stats = state.emulator.GetRuntimeStats();
    const uint64_t displayedRate = state.emulation.cycleAccurate
                                       ? stats.cyclesPerSecond
                                       : stats.instructionsPerSecond;
    const char* unitM = state.emulation.cycleAccurate ? "MHz" : "MIPS";
    const char* unitK = state.emulation.cycleAccurate ? "KHz" : "KIPS";
    const char* unitH = state.emulation.cycleAccurate ? "Hz" : "IPS";
    const auto targetRate = static_cast<uint64_t>(state.emulator.GetTargetIPS());
    const bool isLate = state.emulator.IsRunning() && !state.emulator.IsPaused() &&
                        (displayedRate < static_cast<uint64_t>(static_cast<double>(targetRate) * 0.95));
    ImGui::PushStyleColor(ImGuiCol_Text, isLate ? Theme::RecActive : Theme::AccentPrimary);
    if (displayedRate >= 1000000) {
        ImGui::Text("Sim: %.2f %s", static_cast<double>(displayedRate) / 1000000.0, unitM);
    } else if (displayedRate >= 1000) {
        ImGui::Text("Sim: %.2f %s", static_cast<double>(displayedRate) / 1000.0, unitK);
    } else {
        ImGui::Text("Sim: %llu %s", static_cast<unsigned long long>(displayedRate), unitH);
    }
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        if (ImGui::BeginItemTooltip()) {
            ImGui::Text("Cycles/s: %llu", static_cast<unsigned long long>(stats.cyclesPerSecond));
            if (state.emulator.IsGPUEnabled()) {
                const double gpuPercent = stats.cyclesPerSecond > 0
                                              ? (static_cast<double>(stats.gpuCyclesPerSecond) * 100.0 /
                                                 static_cast<double>(stats.cyclesPerSecond))
                                              : 0.0;
                ImGui::Text("CPU Cycles/s: %llu", static_cast<unsigned long long>(stats.cpuCyclesPerSecond));
                ImGui::Text("GPU Cycles/s: %llu (%.2f%%)", static_cast<unsigned long long>(stats.gpuCyclesPerSecond),
                            gpuPercent);
            }
            ImGui::Text("Instructions/s: %llu", static_cast<unsigned long long>(stats.instructionsPerSecond));
            ImGui::Text("Slices/s: %llu", static_cast<unsigned long long>(stats.slicesPerSecond));
            ImGui::Text("Emulation: %.2f ms", static_cast<double>(stats.emulationNanoseconds) / 1'000'000.0);
            ImGui::Text("Scheduler sleep: %.2f ms", static_cast<double>(stats.sleepNanoseconds) / 1'000'000.0);
            ImGui::Text("Scheduler late: %.2f ms", static_cast<double>(stats.lateNanoseconds) / 1'000'000.0);
            ImGui::EndTooltip();
        }
    }
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
            CustomFileDialog::OpenDialog("RecordSIDDlgKey", "Save Audio Recording", ext, ".", "", 1, nullptr,
                                         ImGuiFileDialogFlags_ConfirmOverwrite);
        } else {
            std::string ext = (state.emulation.recordingSettings.format == VideoFormat::MP4) ? ".mp4" : ".mkv";
            CustomFileDialog::OpenDialog("RecordVideoDlgKey", "Save Video Recording", ext.c_str(), ".", "", 1, nullptr,
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
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::RecActive);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::RecIdle);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderSubtle);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
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
        ImGui::PopStyleColor();
    } else {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    DrawRecordingConfigPopup(state);

    ImGui::EndDisabled();
}

static void DrawIPSSection(AppState& state, float mainColWidth) {
    DrawTargetIPS(state, mainColWidth);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

    ImGui::SameLine();
    DrawActualIPS(state);

    ImGui::SameLine();
    DrawRecordingControls(state);
}

static void HandleCreateSDDialog(AppState& state) {
    if (CustomFileDialog::Display("CreateSDDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            // Ensure .img extension
            if (filePath.size() < 4 || filePath.substr(filePath.size() - 4) != ".img") {
                filePath += ".img";
            }
            std::string tempPath = filePath;
#ifdef TARGET_WASM
            tempPath = "temp.img";
#endif
            if (GUI::CreateFAT16Image(tempPath)) {
#ifdef TARGET_WASM
                std::ifstream ifs(tempPath, std::ios::binary);
                std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                WebFileUtils::download_file("new_sd.img", buffer.data(), buffer.size());
#endif
                if (GUI::IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(filePath);
                } else {
                    state.popups.sdCardDisabled = true;
                }
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleMountSDDialog(AppState& state) {
    if (CustomFileDialog::Display("MountSDDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            if (GUI::IsSDCardEnabled(state)) {
                state.emulator.GetSDCard().Mount(filePath);
            } else {
                state.popups.sdCardDisabled = true;
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleMountFolderDialog(AppState& state) {
    if (CustomFileDialog::Display("MountFolderDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string folderPath = CustomFileDialog::GetFilePathName();
            if (!folderPath.empty() && std::filesystem::is_directory(folderPath)) {
                if (!GUI::IsSDCardEnabled(state)) {
                    state.popups.sdCardDisabled = true;
                } else {
                    if (std::filesystem::exists(std::filesystem::path(folderPath) / ".65C02_SIM_mount")) {
                        GUI::MountFolder(state, folderPath, true);
                    } else {
                        state.popups.sdCardMountSentinelWarning = true;
                        state.popups.sdCardPendingMountFolder = folderPath;
                    }
                }
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleLoadScriptDialog(AppState& state) {
    if (CustomFileDialog::Display("LoadScriptDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            state.script.path = filePath;
            state.script.loaded = true;
            state.script.showConsole = true;
            state.emulator.Pause();  // Pause the frontend before running the script
            state.emulator.GetScriptEngine().LoadAndRun(filePath);
            ImGui::CloseCurrentPopup();
        }
        CustomFileDialog::Close();
    }
}

static void HandleRecordAudioDialog(AppState& state) {
    if (CustomFileDialog::Display("RecordSIDDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            state.emulator.GetSID().StartRecording(filePath);
        }
        CustomFileDialog::Close();
    }
}

static void HandleRecordVideoDialog(AppState& state) {
    if (CustomFileDialog::Display("RecordVideoDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            state.emulation.recordingVideoPath = filePath;
            state.emulation.isRecordingVideo = true;
        }
        CustomFileDialog::Close();
    }
}

static void HandleDialogs(AppState& state) {
    HandleCreateSDDialog(state);
    HandleMountSDDialog(state);
    HandleMountFolderDialog(state);
    HandleLoadScriptDialog(state);
    HandleRecordAudioDialog(state);
    HandleRecordVideoDialog(state);
}

void DrawControlWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47F;
    float controlHeight = std::round((top_section_height - (work_size.y * 0.162F)) * 0.55F);

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, controlHeight), ImGuiCond_Always);
    ImGui::Begin("Control", nullptr, window_flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    DrawControlButtonBar(state);
    DrawDebugMenu(state);
    DrawIPSSection(state, mainColWidth);

    HandleDialogs(state);

    ImGui::SetScrollHereY(1.0F);
    ImGui::End();
}

}  // namespace GUI
