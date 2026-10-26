#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/RegistersWindow.h"
#include "Frontend/GUI/CustomFileDialog.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>
#include <imgui.h>
#include <imgui_internal.h>
#include "Frontend/GUI/Style/Style.h"

using Frontend::CustomFileDialog;

#include "Hardware/Core/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"
#include "Frontend/Control/SDUtils.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/Control/CartridgeUtils.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawRecordingControls(AppState& state);



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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (state.emulator.IsHalted()) {
            ImGui::TextUnformatted("CPU is halted. Reset system to run.");
        } else if (!state.rom.loaded && !state.script.loaded) {
            ImGui::TextUnformatted("Load a ROM or script to start execution.");
        } else if (isPaused) {
            ImGui::TextUnformatted("Resume CPU execution.");
        } else {
            ImGui::TextUnformatted("Pause CPU execution.");
        }
        ImGui::EndTooltip();
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (state.emulator.IsHalted()) {
            ImGui::TextUnformatted("CPU is halted. Reset system to step.");
        } else if (!state.rom.loaded && !state.script.loaded) {
            ImGui::TextUnformatted("Load a ROM or script to step.");
        } else if (!state.emulator.IsPaused()) {
            ImGui::TextUnformatted("Emulator must be paused to use Step.");
        } else {
            ImGui::TextUnformatted("Execute a single CPU instruction.");
        }
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
        if (!state.emulator.IsRunning()) {
            ImGui::TextUnformatted("Rewind unavailable: Emulation has not started.");
        } else if (!state.emulator.IsPaused()) {
            ImGui::TextUnformatted("Rewind unavailable: Emulator must be paused.");
        } else if (!state.emulator.CanRewind()) {
            ImGui::TextUnformatted("Rewind unavailable: No execution history available.");
        } else {
            ImGui::TextUnformatted("Rewind emulation to previous instruction / frame state.");
        }
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    DrawRecordingControls(state);
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
        ImGui::PushItemWidth(150.0F * state.render.uiScale);
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Open settings and debug menu.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        Control::PromptOrExecuteCartridgeAction(state, Control::PendingCartridgeAction::Reset);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Reset CPU, memory, and emulation state.");
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    DrawPlaybackControls(state);

    ImGui::SameLine();
    DrawAudioVideoControls(state);
}

static void DrawTargetIPS(AppState& state) {
    static int s_currentUnit = 0;
    static int s_lastIPS = -1;
    static double s_displayValue = 1.0;

    if (state.emulation.instructionsPerFrame != s_lastIPS) {
        s_lastIPS = state.emulation.instructionsPerFrame;
        double mult = (s_currentUnit == 0) ? 1'000'000.0 : (s_currentUnit == 1 ? 1'000.0 : 1.0);
        s_displayValue = static_cast<double>(s_lastIPS) / mult;
    }

    bool ipsOverridden = state.emulator.GetCartridge().config.targetIPS.has_value();
    ImGui::BeginDisabled(ipsOverridden);

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
    ImGui::TextUnformatted("Target:");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80.0F * state.render.uiScale);
    if (ImGui::InputDouble("##TargetFreq", &s_displayValue, 0.0, 0.0, "%g")) {
        if (s_displayValue > 0.0) {
            while (s_displayValue >= 1000.0 && s_currentUnit > 0) {
                s_displayValue /= 1000.0;
                s_currentUnit--;
            }
            while (s_displayValue < 1.0 && s_currentUnit < 2) {
                s_displayValue *= 1000.0;
                s_currentUnit++;
            }
        }
        double mult = (s_currentUnit == 0) ? 1'000'000.0 : (s_currentUnit == 1 ? 1'000.0 : 1.0);
        int newIPS = std::max(1, static_cast<int>(std::round(s_displayValue * mult)));
        state.emulation.instructionsPerFrame = newIPS;
        state.emulator.SetTargetIPS(newIPS);
        s_lastIPS = newIPS;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(95.0F * state.render.uiScale);
    const char* unitsHz[] = { "MHz", "KHz", "Hz" };
    const char* unitsIPS[] = { "MIPS", "KIPS", "IPS" };
    const char** units = state.emulation.cycleAccurate ? unitsHz : unitsIPS;
    int prevUnit = s_currentUnit;
    if (ImGui::Combo("##TargetUnit", &s_currentUnit, units, 3)) {
        if (s_currentUnit != prevUnit) {
            double prevMult = (prevUnit == 0) ? 1'000'000.0 : (prevUnit == 1 ? 1'000.0 : 1.0);
            double newMult = (s_currentUnit == 0) ? 1'000'000.0 : (s_currentUnit == 1 ? 1'000.0 : 1.0);
            s_displayValue = (s_displayValue * prevMult) / newMult;
        }
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
            ImGui::TextUnformatted("Simulation Rate");
            ImGui::Separator();
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

            ImGui::Spacing();
            ImGui::TextUnformatted("Batch & Components");
            ImGui::Separator();
            const double avgBatchUs = static_cast<double>(stats.avgBatchDurationNs) / 1000.0;
            const double lastBatchUs = static_cast<double>(stats.lastBatchDurationNs) / 1000.0;
            ImGui::Text("Avg Batch Time: %.2f us", avgBatchUs);
            ImGui::Text("Last Batch Time: %.2f us", lastBatchUs);
            const uint64_t refCycles = stats.avgBatchCycles > 0 ? stats.avgBatchCycles : stats.lastBatchCycles;
            ImGui::Text("Batch Cycles: %llu", static_cast<unsigned long long>(refCycles));
            ImGui::Text("Batches/s: %llu", static_cast<unsigned long long>(stats.batchesPerSecond));

            std::vector<std::string> activeComponents;
            std::unordered_set<const Hardware::IBusDevice*> seenDevices;
            for (const auto& reg : state.emulator.GetMem().GetRegisteredDevices()) {
                if (reg.enabled && reg.device && !seenDevices.contains(reg.device)) {
                    seenDevices.insert(reg.device);
                    activeComponents.push_back(reg.device->GetName());
                }
            }

            const size_t compCount = std::max<size_t>(1, activeComponents.size());
            const uint64_t refBatchNs = stats.avgBatchDurationNs > 0 ? stats.avgBatchDurationNs : stats.lastBatchDurationNs;
            const double timePerCompBatchUs = (static_cast<double>(refBatchNs) / 1000.0) / static_cast<double>(compCount);
            const double timePerCompCycleNs = refCycles > 0
                                                  ? (static_cast<double>(refBatchNs) /
                                                     (static_cast<double>(compCount) * static_cast<double>(refCycles)))
                                                  : 0.0;

            std::string compListStr;
            for (size_t i = 0; i < activeComponents.size(); ++i) {
                if (i > 0) {
                    compListStr += ", ";
                }
                compListStr += activeComponents[i];
            }
            ImGui::Text("Active Bus Devices (%zu): %s", activeComponents.size(), compListStr.c_str());
            ImGui::Text("Avg / Component (Batch): %.2f us", timePerCompBatchUs);
            ImGui::Text("Avg / Component (Cycle): %.2f ns", timePerCompCycleNs);

            ImGui::Spacing();
            ImGui::TextUnformatted("Scheduler (Last 1s)");
            ImGui::Separator();
            const double emuMs = static_cast<double>(stats.emulationNanoseconds) / 1'000'000.0;
            const double sleepMs = static_cast<double>(stats.sleepNanoseconds) / 1'000'000.0;
            const double lateMs = static_cast<double>(stats.lateNanoseconds) / 1'000'000.0;
            ImGui::Text("Emulation: %.2f ms (%.1f%% load)", emuMs, emuMs / 10.0);
            ImGui::Text("Sleep: %.2f ms (%.1f%% idle)", sleepMs, sleepMs / 10.0);
            ImGui::Text("Late: %.2f ms", lateMs);
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
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canStart) {
            ImGui::TextUnformatted("Select at least one video track to record.");
        } else {
            ImGui::TextUnformatted("Start recording to file with current settings.");
        }
        ImGui::EndTooltip();
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
#endif

    if (isRecording) {
        ImGui::PopStyleColor();
    } else {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
#ifdef TARGET_WASM
        ImGui::TextUnformatted("Recording is only supported in local builds.");
#else
        if (!state.rom.loaded) {
            ImGui::TextUnformatted("Load a ROM to enable recording.");
        } else if (isRecording) {
            ImGui::TextUnformatted("Stop active recording.");
        } else {
            ImGui::TextUnformatted("Configure and start audio/video recording.");
        }
#endif
        ImGui::EndTooltip();
    }

    DrawRecordingConfigPopup(state);

    ImGui::EndDisabled();
}

static void DrawIPSSection(AppState& state) {
    DrawTargetIPS(state);

    ImGui::SameLine(0.0F, 14.0F);
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

    ImGui::SameLine(0.0F, 14.0F);
    ImGui::AlignTextToFramePadding();
    DrawActualIPS(state);
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
            std::string folderPath = CustomFileDialog::GetCurrentPath();
            const std::filesystem::path hostFolder(std::u8string(folderPath.begin(), folderPath.end()));
            if (!folderPath.empty() && std::filesystem::is_directory(hostFolder)) {
                if (!GUI::IsSDCardEnabled(state)) {
                    state.popups.sdCardDisabled = true;
                } else {
                    if (std::filesystem::exists(hostFolder / ".65C02_SIM_mount")) {
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
    float lcdHeight = work_size.y * 0.144F;
    float controlHeight = top_section_height - lcdHeight;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, controlHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0F * state.render.uiScale, 10.0F * state.render.uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0F * state.render.uiScale, 6.0F * state.render.uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0F * state.render.uiScale, 5.0F * state.render.uiScale));
    ImGui::Begin("Control", nullptr, window_flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    DrawControlButtonBar(state);
    DrawDebugMenu(state);

    float y1 = ImGui::GetCursorPosY();
    const float kRegHeight = ImGui::GetFrameHeight();
    float regPosY = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - kRegHeight;
    float sep2PosY = regPosY - ImGui::GetStyle().ItemSpacing.y - 4.0F;
    float midSpace = sep2PosY - y1;
    float extraSpace = midSpace - (ImGui::GetFrameHeight() + 1.0F);
    float gap = std::max(4.0F, extraSpace / 3.0F);

    float sep1PosY = y1 + gap;
    ImGui::SetCursorPosY(sep1PosY);
    ImGui::Separator();

    ImGui::SetCursorPosY(sep1PosY + 1.0F + gap);
    DrawIPSSection(state);

    ImGui::SetCursorPosY(sep2PosY);
    ImGui::Separator();

    ImGui::SetCursorPosY(regPosY);
    DrawRegistersSection(state);

    HandleDialogs(state);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

}  // namespace GUI
