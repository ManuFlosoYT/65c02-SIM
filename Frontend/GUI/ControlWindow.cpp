#include "Frontend/GUI/ControlWindow.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"
#include "Frontend/GUI/SDUtils.h"
#include "Hardware/Core/Emulator.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

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
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleMountSDDialog(AppState& state) {
    if (ImGuiFileDialog::Instance()->Display("MountSDDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            if (GUI::IsSDCardEnabled(state)) {
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
    DrawDebugMenu(state);
    DrawIPSSection(state, mainColWidth);

    HandleDialogs(state);

    ImGui::SetScrollHereY(1.0F);
    ImGui::End();
}

}  // namespace GUI
