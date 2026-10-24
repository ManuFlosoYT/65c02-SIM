#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "Frontend/GUI/MIDI/MIDIAudioEngine.h"
#include "Frontend/GUI/MIDI/MIDIFile.h"
#include "Frontend/GUI/MIDI/MIDISoundBank.h"
#include "Frontend/GUI/Style/Style.h"
#include "Frontend/GUI/CustomFileDialog.h"

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI {

namespace {

constexpr int MIN_PITCH = 12;
constexpr int MAX_PITCH = 107;
constexpr int NUM_PITCHES = MAX_PITCH - MIN_PITCH + 1;
constexpr std::size_t MAX_UNDO_STATES = 32;

constexpr std::array<ImU32, 3> SID_VOICE_COLORS = {
    IM_COL32(79, 195, 247, 230),
    IM_COL32(255, 183, 77, 230),
    IM_COL32(240, 98, 146, 230)
};
constexpr ImU32 SID_CULLED_COLOR = IM_COL32(185, 70, 70, 165);

enum class DragMode : std::uint8_t {
    None = 0,
    CreateNote,
    MoveNote,
    ResizeNote,
    BoxSelect,
    ScrubTimeline,
    VelocityEdit,
    PianoKeyAudition
};

struct MIDIWorkspaceState {
    bool initialized = false;
    MIDI::MIDIProject project;
    MIDI::MIDIAudioEngine audioEngine;
    std::size_t selectedTrackIndex = 0;

    std::vector<MIDI::MIDIProject> undoStack;
    std::vector<MIDI::MIDIProject> redoStack;
    std::vector<MIDI::MIDINote> copiedNotes;

    float pixelsPerBeat = 80.0F;
    float rowHeight = 16.0F;
    double snapBeats = 0.25;
    double defaultNoteDurationBeats = 0.5;
    bool showGhostNotes = true;
    bool colorBySidVoice = true;
    float scrollX = 0.0F;
    float scrollY = 560.0F;
    bool initialScrollSet = false;

    DragMode dragMode = DragMode::None;
    std::uint32_t activeNoteId = 0;
    double dragStartBeat = 0.0;
    int dragStartPitch = 60;
    double noteOrigStartBeat = 0.0;
    double noteOrigDurationBeats = 0.5;
    int noteOrigPitch = 60;
    int lastAuditionedPitch = -1;
    ImVec2 boxSelectStart{0.0F, 0.0F};
    ImVec2 boxSelectEnd{0.0F, 0.0F};

    std::array<char, 128> trackNameBuf{};
    std::size_t lastSyncedTrackIdx = 999999;

    std::vector<std::string> presetMidiFiles;
    bool presetsScanned = false;

    std::string statusLog = "Ready — Click Piano Roll to place & audition SID notes.";
};

MIDIWorkspaceState& GetWorkspaceState() {
    static MIDIWorkspaceState ws;
    return ws;
}

void DrawColoredText(const ImVec4& color, std::string_view text) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}

void ShowTextTooltip(std::string_view text) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::EndTooltip();
    }
}

[[nodiscard]] std::string PitchToNoteName(int pitch) {
    static constexpr std::array<const char*, 12> NOTE_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    const int clamped = std::clamp(pitch, 0, 127);
    const int octave = (clamped / 12) - 1;
    const auto noteIdx = static_cast<std::size_t>(clamped % 12);
    return std::format("{}{}", NOTE_NAMES.at(noteIdx), octave);
}

[[nodiscard]] bool IsBlackKey(int pitch) {
    const int semitone = pitch % 12;
    return semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10;
}

[[nodiscard]] MIDI::MIDITrackRole GetEffectiveTrackRole(const MIDI::MIDITrack& track) {
    if (track.channel == 9 || track.role == MIDI::MIDITrackRole::Percussion) {
        return MIDI::MIDITrackRole::Percussion;
    }
    if (track.isMainTrack || track.role == MIDI::MIDITrackRole::Melody) {
        return MIDI::MIDITrackRole::Melody;
    }
    if (track.role == MIDI::MIDITrackRole::Bass) {
        return MIDI::MIDITrackRole::Bass;
    }
    if (track.role == MIDI::MIDITrackRole::Harmony) {
        return MIDI::MIDITrackRole::Harmony;
    }
    if (track.notes.empty()) {
        return MIDI::MIDITrackRole::Harmony;
    }
    double sumPitch = 0.0;
    for (const auto& n : track.notes) {
        sumPitch += static_cast<double>(n.pitch);
    }
    const double avgPitch = sumPitch / static_cast<double>(track.notes.size());
    return avgPitch < 50.0 ? MIDI::MIDITrackRole::Bass : MIDI::MIDITrackRole::Harmony;
}

[[nodiscard]] const char* GetRoleBadgeText(MIDI::MIDITrackRole role) {
    switch (role) {
        case MIDI::MIDITrackRole::Melody:
            return "MELODY";
        case MIDI::MIDITrackRole::Bass:
            return "BASS";
        case MIDI::MIDITrackRole::Percussion:
            return "DRUMS";
        case MIDI::MIDITrackRole::Harmony:
        case MIDI::MIDITrackRole::Auto:
        default:
            return "HARMONY";
    }
}

[[nodiscard]] ImVec4 GetRoleBadgeColor(MIDI::MIDITrackRole role) {
    switch (role) {
        case MIDI::MIDITrackRole::Melody:
            return ImVec4(0.98F, 0.78F, 0.18F, 1.0F);
        case MIDI::MIDITrackRole::Bass:
            return ImVec4(0.31F, 0.76F, 0.97F, 1.0F);
        case MIDI::MIDITrackRole::Percussion:
            return ImVec4(0.94F, 0.38F, 0.57F, 1.0F);
        case MIDI::MIDITrackRole::Harmony:
        case MIDI::MIDITrackRole::Auto:
        default:
            return ImVec4(0.55F, 0.65F, 0.78F, 1.0F);
    }
}

void PushUndoState(MIDIWorkspaceState& ws) {
    if (ws.undoStack.size() >= MAX_UNDO_STATES) {
        ws.undoStack.erase(ws.undoStack.begin());
    }
    ws.undoStack.push_back(ws.project);
    ws.redoStack.clear();
}

void RecomputeAllocation(MIDIWorkspaceState& ws, Control::AppState& state) {
    MIDI::MIDIFile::ComputeVoicePreviewAllocation(ws.project, state.emulator.GetSID().GetModel());
    const double totalBeats = std::max(16.0, std::ceil(ws.project.GetTotalDurationBeats() / 4.0) * 4.0);
    ws.audioEngine.SetLoopRegion(0.0, totalBeats);
}

void PerformUndo(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.undoStack.empty()) {
        return;
    }
    ws.redoStack.push_back(ws.project);
    ws.project = std::move(ws.undoStack.back());
    ws.undoStack.pop_back();
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        ws.selectedTrackIndex = ws.project.tracks.empty() ? 0 : ws.project.tracks.size() - 1;
    }
    ws.lastSyncedTrackIdx = 999999;
    RecomputeAllocation(ws, state);
    ws.statusLog = "Undo performed.";
}

void PerformRedo(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.redoStack.empty()) {
        return;
    }
    ws.undoStack.push_back(ws.project);
    ws.project = std::move(ws.redoStack.back());
    ws.redoStack.pop_back();
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        ws.selectedTrackIndex = ws.project.tracks.empty() ? 0 : ws.project.tracks.size() - 1;
    }
    ws.lastSyncedTrackIdx = 999999;
    RecomputeAllocation(ws, state);
    ws.statusLog = "Redo performed.";
}

void CopySelectedNotes(MIDIWorkspaceState& ws) {
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        return;
    }
    const auto& track = ws.project.tracks.at(ws.selectedTrackIndex);
    ws.copiedNotes.clear();
    double minBeat = 1e9;
    for (const auto& note : track.notes) {
        if (note.selected) {
            ws.copiedNotes.push_back(note);
            minBeat = std::min(minBeat, note.startBeat);
        }
    }
    if (!ws.copiedNotes.empty()) {
        for (auto& note : ws.copiedNotes) {
            note.startBeat -= minBeat;
        }
        ws.statusLog = std::format("Copied {} note(s) to clipboard.", ws.copiedNotes.size());
    }
}

void PasteNotes(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.copiedNotes.empty() || ws.selectedTrackIndex >= ws.project.tracks.size()) {
        return;
    }
    PushUndoState(ws);
    auto& track = ws.project.tracks.at(ws.selectedTrackIndex);
    for (auto& note : track.notes) {
        note.selected = false;
    }
    const double baseBeat = ws.audioEngine.GetPlayheadBeats();
    for (const auto& src : ws.copiedNotes) {
        MIDI::MIDINote pasted = src;
        pasted.id = ws.project.nextNoteId++;
        pasted.startBeat = std::max(0.0, baseBeat + src.startBeat);
        pasted.selected = true;
        track.notes.push_back(pasted);
    }
    RecomputeAllocation(ws, state);
    ws.statusLog = std::format("Pasted {} note(s) at beat {:.2f}.", ws.copiedNotes.size(), baseBeat);
}

void DeleteSelectedNotes(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        return;
    }
    auto& track = ws.project.tracks.at(ws.selectedTrackIndex);
    const bool anySelected = std::ranges::any_of(track.notes, [](const MIDI::MIDINote& n) { return n.selected; });
    if (!anySelected) {
        return;
    }
    PushUndoState(ws);
    const auto oldCount = track.notes.size();
    std::erase_if(track.notes, [](const MIDI::MIDINote& n) { return n.selected; });
    const auto removed = oldCount - track.notes.size();
    RecomputeAllocation(ws, state);
    ws.statusLog = std::format("Deleted {} selected note(s).", removed);
}

[[nodiscard]] double SnapBeatValue(double beat, double snapBeats) {
    if (snapBeats <= 0.0) {
        return std::max(0.0, beat);
    }
    return std::max(0.0, std::round(beat / snapBeats) * snapBeats);
}

[[nodiscard]] double FloorSnapBeatValue(double beat, double snapBeats) {
    if (snapBeats <= 0.0) {
        return std::max(0.0, beat);
    }
    return std::max(0.0, std::floor(beat / snapBeats) * snapBeats);
}

void EnsureWorkspaceInitialized(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.initialized) {
        return;
    }
    MIDI::MIDISoundBank::EnsureLoaded();
    ws.project.InitDefaultProject();
    ws.selectedTrackIndex = 0;
    ws.initialized = true;
    RecomputeAllocation(ws, state);
}

void ScanPresetMidis(MIDIWorkspaceState& ws) {
    if (ws.presetsScanned) {
        return;
    }
    ws.presetsScanned = true;
    ws.presetMidiFiles.clear();
    std::error_code ec;
    if (std::filesystem::exists("assets/midi", ec)) {
        for (const auto& entry : std::filesystem::directory_iterator("assets/midi", ec)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::string ext = entry.path().extension().string();
            if (ext == ".mid" || ext == ".midi" || ext == ".MID" || ext == ".MIDI") {
                ws.presetMidiFiles.push_back(entry.path().string());
            }
        }
        std::ranges::sort(ws.presetMidiFiles);
    }
}

#ifndef TARGET_WASM
[[nodiscard]] std::vector<std::uint8_t> ReadBinaryFileBytes(const std::string& path) {
    std::ifstream inFile(path, std::ios::binary | std::ios::ate);
    if (!inFile) {
        return {};
    }
    const auto fileSize = inFile.tellg();
    if (fileSize <= 0) {
        return {};
    }
    inFile.seekg(0, std::ios::beg);
    std::vector<char> buf(static_cast<std::size_t>(fileSize));
    inFile.read(buf.data(), fileSize);
    return {buf.begin(), buf.end()};
}

[[nodiscard]] std::string ReadTextFileContent(const std::string& path) {
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile) {
        return {};
    }
    return {std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::string SanitizeCleanStem(const std::string& path) {
    std::string stem = std::filesystem::path(path).stem().string();
    std::erase(stem, ' ');
    if (stem.empty()) {
        stem = "ide_preview";
    }
    return stem;
}

bool CompileMIDIProjectToBinary(MIDIWorkspaceState& ws, Control::AppState& state, bool microDosSid,
                                std::vector<std::uint8_t>& outBinary, std::string& outSavedPath) {
    std::error_code ec;
    std::filesystem::create_directories("tools/sid/build", ec);
    std::filesystem::create_directories("output/midi", ec);

    std::string midPath = ws.project.filePath;
    if (midPath.empty()) {
        midPath = "tools/sid/build/ide_preview.mid";
    }
    std::string saveErr;
    if (!MIDI::MIDIFile::SaveToFile(midPath, ws.project, saveErr)) {
        ws.statusLog = "Failed to save temporary MIDI: " + saveErr;
        return false;
    }

    const std::string cleanName = SanitizeCleanStem(midPath);
    const std::string chipStr = (state.emulator.GetSID().GetModel() == Hardware::SIDModel::MOS6581) ? "6581" : "8580";
    const std::string sysStr = ws.audioEngine.IsNTSC() ? "NTSC" : "PAL";
    const int mainTrackIdx = ws.project.GetMainTrackIndex();

    std::string extraArgs = std::format("--chip {} --system {}", chipStr, sysStr);
    if (mainTrackIdx >= 0 && static_cast<std::size_t>(mainTrackIdx) < ws.project.tracks.size()) {
        const auto& mainTrk = ws.project.tracks.at(static_cast<std::size_t>(mainTrackIdx));
        extraArgs += std::format(" --main-track {} --melody-channel {}", mainTrackIdx + 1, mainTrk.channel);
    }
    for (const auto& trk : ws.project.tracks) {
        if (trk.role == MIDI::MIDITrackRole::Bass && trk.channel != 9) {
            extraArgs += std::format(" --bass-channel {}", trk.channel);
            break;
        }
    }

    if (microDosSid) {
        const std::string outSidPath = "output/midi/" + cleanName + ".sid";
        std::filesystem::remove(outSidPath, ec);
        std::string cmdOut;
        const std::string cmd =
            std::format("python3 tools/sid/audio_to_sid.py \"{}\" --mode max_quality --microDOS {}", midPath, extraArgs);
        const int rc = CompilerFrontend::RunCommand(cmd, cmdOut);
        if (rc == 0 && std::filesystem::exists(outSidPath, ec)) {
            outBinary = ReadBinaryFileBytes(outSidPath);
            outSavedPath = outSidPath;
            ws.statusLog = std::format("Built MicroDOS .sid ({} bytes): {}", outBinary.size(), outSidPath);
            return !outBinary.empty();
        }
        ws.statusLog = "audio_to_sid.py (--microDOS) failed. Check console/output.";
        state.ide.outputLog = cmdOut;
        return false;
    }

    const std::string outBinPath = "output/midi/" + cleanName + ".bin";
    std::filesystem::remove(outBinPath, ec);

    std::string scriptOut;
    const std::string scriptCmd = std::format("bash sdk/scripts/midi-to-bin.sh \"{}\" {}", midPath, extraArgs);
    const int scriptRc = CompilerFrontend::RunCommand(scriptCmd, scriptOut);
    state.ide.outputLog = scriptOut;

    if (scriptRc == 0 && std::filesystem::exists(outBinPath, ec)) {
        outBinary = ReadBinaryFileBytes(outBinPath);
        if (!outBinary.empty()) {
            outSavedPath = outBinPath;
            ws.statusLog = std::format("midi-to-bin.sh succeeded ({} bytes): {}", outBinary.size(), outBinPath);
            return true;
        }
    }

    static constexpr std::array<const char*, 5> FALLBACK_MODES = {
        "max_quality", "quality", "balanced", "compressed", "max_compression"
    };
    const std::string asmPath = "tools/sid/build/" + cleanName + ".s";
    for (const char* mode : FALLBACK_MODES) {
        std::filesystem::remove(asmPath, ec);
        std::string pyOut;
        const std::string pyCmd =
            std::format("python3 tools/sid/audio_to_sid.py \"{}\" --mode {} {}", midPath, mode, extraArgs);
        const int pyRc = CompilerFrontend::RunCommand(pyCmd, pyOut);
        state.ide.outputLog += pyOut;
        if (pyRc != 0 || !std::filesystem::exists(asmPath, ec)) {
            continue;
        }
        const std::string asmCode = ReadTextFileContent(asmPath);
        if (asmCode.empty()) {
            continue;
        }
        auto buildRes = CompilerFrontend::Compile(CompilerFrontend::BuildType::Assembly, asmCode, cleanName + ".s");
        state.ide.outputLog += buildRes.log;
        if (buildRes.success && !buildRes.binary.empty()) {
            outBinary = std::move(buildRes.binary);
            std::ofstream binOut(outBinPath, std::ios::binary);
            if (binOut) {
                for (const auto byteVal : outBinary) {
                    binOut.put(static_cast<char>(byteVal));
                }
            }
            outSavedPath = outBinPath;
            ws.statusLog = std::format("Compiled SID ROM [mode: {}] ({} bytes).", mode, outBinary.size());
            return true;
        }
    }

    ws.statusLog = "Failed to compile MIDI to 65C02 ROM.";
    return false;
}

void HandleCompileAndRunROM(MIDIWorkspaceState& ws, Control::AppState& state) {
    ws.audioEngine.Stop(state);
    std::vector<std::uint8_t> binary;
    std::string binPath;
    if (!CompileMIDIProjectToBinary(ws, state, false, binary, binPath)) {
        return;
    }

    state.emulator.Pause();
    state.emulator.SetCartridge(Core::Cartridge{});
    state.emulator.GetSID().EnableSound(true);
    state.emulator.SetSIDEnabled(true);

    std::string errMsg;
    const std::string romName = std::filesystem::path(binPath).filename().string();
    if (state.emulator.InitFromMemory(binary, romName, errMsg)) {
        state.rom.bin = romName;
        state.rom.data = binary;
        state.rom.loaded = true;
        state.emulator.ClearProfiler();
        state.emulator.Resume();
        state.ide.open = false;
        ws.statusLog = "Loaded & running on 65C02 Emulator: " + romName;
    } else {
        ws.statusLog = "Emulator load failed: " + errMsg;
    }
}
#endif

void DrawTopRowModeAndFileBar(MIDIWorkspaceState& ws, Control::AppState& state) {
    DrawColoredText(Theme::AccentPrimary, "MODE:");
    ImGui::SameLine();
    if (ImGui::RadioButton("C", state.ide.mode == Control::IDEMode::C)) {
        ws.audioEngine.Stop(state);
        state.ide.mode = Control::IDEMode::C;
        state.ide.isCMode = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ASM", state.ide.mode == Control::IDEMode::ASM)) {
        ws.audioEngine.Stop(state);
        state.ide.mode = Control::IDEMode::ASM;
        state.ide.isCMode = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MicroDOS (C)", state.ide.mode == Control::IDEMode::MicroDOS)) {
        ws.audioEngine.Stop(state);
        state.ide.mode = Control::IDEMode::MicroDOS;
        state.ide.isCMode = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("MIDI", state.ide.mode == Control::IDEMode::MIDI)) {
        state.ide.mode = Control::IDEMode::MIDI;
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();

    if (ImGui::SmallButton("New")) {
        ws.audioEngine.Stop(state);
        PushUndoState(ws);
        ws.project.InitDefaultProject();
        ws.selectedTrackIndex = 0;
        ws.lastSyncedTrackIdx = 999999;
        RecomputeAllocation(ws, state);
        ws.statusLog = "Initialized new 3-track SID MIDI project.";
    }
    ShowTextTooltip("Create a new default 3-track SID MIDI project (Lead, Bass, Drums)");

    ImGui::SameLine();
    if (ImGui::SmallButton("Open .mid")) {
        Frontend::CustomFileDialog::OpenDialog("MIDI_OpenFile", "Open MIDI File", ".mid,.midi,.*", ".");
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Preset MIDIs")) {
        ws.presetsScanned = false;
        ScanPresetMidis(ws);
        ImGui::OpenPopup("MIDI_PresetsPopup");
    }
    if (ImGui::BeginPopup("MIDI_PresetsPopup")) {
        DrawColoredText(Theme::AccentPrimary, "Bundled Songs (assets/midi/):");
        ImGui::Separator();
        if (ws.presetMidiFiles.empty()) {
            ImGui::TextDisabled("No .mid files found in assets/midi/");
        } else {
            for (const auto& path : ws.presetMidiFiles) {
                const std::string filename = std::filesystem::path(path).filename().string();
                if (ImGui::Selectable(filename.c_str())) {
                    OpenMIDIFileInWorkspace(state, path);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Save")) {
        if (ws.project.filePath.empty()) {
            Frontend::CustomFileDialog::OpenDialog("MIDI_SaveFile", "Save MIDI File As", ".mid,.midi", ".");
        } else {
            std::string err;
            if (MIDI::MIDIFile::SaveToFile(ws.project.filePath, ws.project, err)) {
                ws.statusLog = "Saved MIDI to " + ws.project.filePath;
            } else {
                ws.statusLog = "Save error: " + err;
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Save As")) {
        Frontend::CustomFileDialog::OpenDialog("MIDI_SaveFile", "Save MIDI File As", ".mid,.midi", ".");
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::RecIdle);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::RecActive);
    if (ImGui::SmallButton("Close IDE")) {
        ws.audioEngine.Stop(state);
        state.ide.open = false;
    }
    ImGui::PopStyleColor(2);
}

void DrawSecondRowTransportAndPipelineBar(MIDIWorkspaceState& ws, Control::AppState& state) {
    const bool isPlaying = ws.audioEngine.IsPlaying();
    if (isPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentActive);
        ImGui::PushStyleColor(ImGuiCol_Text, ImColor(Theme::LcdPetroleumBg).Value);
        if (ImGui::SmallButton("|| Pause")) {
            ws.audioEngine.Pause(state);
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::RunBg);
        if (ImGui::SmallButton("> Play")) {
            ws.audioEngine.Play(state, ws.project);
        }
        ImGui::PopStyleColor();
    }
    ShowTextTooltip("Play / Pause live SID sequencer (Shortcut: Space)");

    ImGui::SameLine();
    if (ImGui::SmallButton("[] Stop")) {
        ws.audioEngine.Stop(state);
    }

    ImGui::SameLine();
    bool loop = ws.audioEngine.IsLoopEnabled();
    if (ImGui::Checkbox("Loop", &loop)) {
        ws.audioEngine.SetLoopEnabled(loop);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(58.0F);
    auto bpmFloat = static_cast<float>(ws.project.bpm);
    if (ImGui::DragFloat("BPM", &bpmFloat, 0.5F, 20.0F, 300.0F, "%.0f")) {
        ws.project.bpm = static_cast<double>( std::clamp(bpmFloat, 20.0F, 300.0F));
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.0F);
    static constexpr std::array<const char*, 5> SNAP_LABELS = {"1/4", "1/8", "1/16", "1/32", "Free"};
    static constexpr std::array<double, 5> SNAP_VALUES = {1.0, 0.5, 0.25, 0.125, 0.0};
    int currentSnapIdx = 2;
    for (std::size_t idx = 0; idx < SNAP_VALUES.size(); ++idx) {
        if (std::abs(ws.snapBeats - SNAP_VALUES.at(idx)) < 1e-4) {
            currentSnapIdx = static_cast<int>(idx);
            break;
        }
    }
    if (ImGui::Combo("Snap", &currentSnapIdx, SNAP_LABELS.data(), static_cast<int>(SNAP_LABELS.size()))) {
        ws.snapBeats = SNAP_VALUES.at(static_cast<std::size_t>(currentSnapIdx));
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(74.0F);
    int chipIdx = (state.emulator.GetSID().GetModel() == Hardware::SIDModel::MOS6581) ? 0 : 1;
    static constexpr std::array<const char*, 2> CHIP_LABELS = {"6581", "8580"};
    if (ImGui::Combo("##ChipModel", &chipIdx, CHIP_LABELS.data(), static_cast<int>(CHIP_LABELS.size()))) {
        const auto newModel = (chipIdx == 0) ? Hardware::SIDModel::MOS6581 : Hardware::SIDModel::MOS8580;
        state.emulator.GetSID().SetModel(newModel);
        RecomputeAllocation(ws, state);
    }
    ShowTextTooltip("Target SID Chip Model (6581 Warm Analog vs 8580 Crisp Clean Mixed Waveforms)");

    ImGui::SameLine();
    const bool isNtsc = ws.audioEngine.IsNTSC();
    if (ImGui::SmallButton(isNtsc ? "NTSC" : "PAL")) {
        ws.audioEngine.SetNTSC(!isNtsc);
    }
    ShowTextTooltip("Toggle SID Clock Standard (NTSC 1.000 MHz / PAL 985.248 kHz)");

#ifndef TARGET_WASM
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentActive);
    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(Theme::LcdPetroleumBg).Value);
    if (ImGui::SmallButton("Compile & Run ROM")) {
        HandleCompileAndRunROM(ws, state);
    }
    ImGui::PopStyleColor(2);
    ShowTextTooltip("Convert MIDI via audio_to_sid.py + midi-to-bin.sh (--main-track) and run on 65C02 Simulator");

    ImGui::SameLine();
    if (ImGui::SmallButton("Export .bin/.sid")) {
        Frontend::CustomFileDialog::OpenDialog("MIDI_ExportBinSid", "Export Compiled .bin or .sid", ".bin,.sid", ".");
    }
    ShowTextTooltip("Export standalone 65C02 ROM (.bin) or MicroDOS SDZ v2 stream (.sid)");
#endif
}

void DrawTrackListPanel(MIDIWorkspaceState& ws, Control::AppState& state, float width, float height) {
    if (!ImGui::BeginChild("##MIDITrackList", ImVec2(width, height), true)) {
        ImGui::EndChild();
        return;
    }

    DrawColoredText(Theme::AccentPrimary, "TRACKS & MAIN ROLE");
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Track")) {
        PushUndoState(ws);
        MIDI::MIDITrack newTrack;
        const int nextNum = static_cast<int>(ws.project.tracks.size()) + 1;
        newTrack.name = std::format("Track {}", nextNum);
        int freeChan = 0;
        for (int candidate = 0; candidate < 16; ++candidate) {
            if (candidate == 9) {
                continue;
            }
            const bool used = std::ranges::any_of(
                ws.project.tracks, [candidate](const MIDI::MIDITrack& t) { return t.channel == candidate; });
            if (!used) {
                freeChan = candidate;
                break;
            }
        }
        newTrack.channel = freeChan;
        newTrack.program = 80;
        ws.project.tracks.push_back(std::move(newTrack));
        ws.selectedTrackIndex = ws.project.tracks.size() - 1;
        ws.lastSyncedTrackIdx = 999999;
        RecomputeAllocation(ws, state);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(ws.project.tracks.size() <= 1);
    if (ImGui::SmallButton("- Track")) {
        PushUndoState(ws);
        ws.project.tracks.erase(ws.project.tracks.begin() + static_cast<std::ptrdiff_t>(ws.selectedTrackIndex));
        if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
            ws.selectedTrackIndex = ws.project.tracks.size() - 1;
        }
        ws.lastSyncedTrackIdx = 999999;
        RecomputeAllocation(ws, state);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(ws.undoStack.empty());
    if (ImGui::SmallButton("Undo")) {
        PerformUndo(ws, state);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(ws.redoStack.empty());
    if (ImGui::SmallButton("Redo")) {
        PerformRedo(ws, state);
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    for (std::size_t idx = 0; idx < ws.project.tracks.size(); ++idx) {
        auto& track = ws.project.tracks.at(idx);
        ImGui::PushID(static_cast<int>(idx));

        const bool isMain = track.isMainTrack;
        if (isMain) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95F, 0.72F, 0.12F, 0.95F));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00F, 0.82F, 0.22F, 1.00F));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08F, 0.08F, 0.08F, 1.00F));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::SurfaceHover);
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
        }
        if (ImGui::SmallButton(isMain ? "* Main" : "o Main")) {
            PushUndoState(ws);
            if (isMain) {
                track.isMainTrack = false;
                if (track.role == MIDI::MIDITrackRole::Melody) {
                    track.role = MIDI::MIDITrackRole::Auto;
                }
            } else {
                ws.project.SetMainTrack(idx);
            }
            RecomputeAllocation(ws, state);
        }
        ImGui::PopStyleColor(3);
        ShowTextTooltip(
            "Mark as Pista Principal (Main Melody Track): Gets highest SID voice priority (+2500), Vibrato LFO, and "
            "exports with --main-track to midi-to-bin.sh");

        ImGui::SameLine(0.0F, 3.0F);
        if (track.muted) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85F, 0.20F, 0.20F, 0.9F));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        }
        if (ImGui::SmallButton("M")) {
            track.muted = !track.muted;
            RecomputeAllocation(ws, state);
        }
        ImGui::PopStyleColor();
        ShowTextTooltip("Mute Track");

        ImGui::SameLine(0.0F, 2.0F);
        if (track.solo) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90F, 0.70F, 0.10F, 0.9F));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Surface);
        }
        if (ImGui::SmallButton("S")) {
            track.solo = !track.solo;
            RecomputeAllocation(ws, state);
        }
        ImGui::PopStyleColor();
        ShowTextTooltip("Solo Track");

        ImGui::SameLine(0.0F, 5.0F);
        const auto effRole = GetEffectiveTrackRole(track);
        const std::string label =
            std::format("{} [Ch{}|{}] ({})", track.name, track.channel + 1, GetRoleBadgeText(effRole), track.notes.size());
        const bool isSelected = (ws.selectedTrackIndex == idx);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            ws.selectedTrackIndex = idx;
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void DrawTrackInspectorPanel(MIDIWorkspaceState& ws, Control::AppState& state, float width, float height) {
    if (!ImGui::BeginChild("##MIDITrackInspector", ImVec2(width, height), true)) {
        ImGui::EndChild();
        return;
    }

    if (ws.project.tracks.empty()) {
        ImGui::TextDisabled("No tracks in project.");
        ImGui::EndChild();
        return;
    }
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        ws.selectedTrackIndex = 0;
    }

    auto& track = ws.project.tracks.at(ws.selectedTrackIndex);
    if (ws.lastSyncedTrackIdx != ws.selectedTrackIndex) {
        ws.trackNameBuf.fill('\0');
        const std::size_t copyLen = std::min(track.name.size(), ws.trackNameBuf.size() - 1);
        std::copy_n(track.name.data(), copyLen, ws.trackNameBuf.data());
        ws.lastSyncedTrackIdx = ws.selectedTrackIndex;
    }

    DrawColoredText(Theme::AccentPrimary, "TRACK & SID INSTRUMENT BANK");
    ImGui::Separator();

    ImGui::SetNextItemWidth(130.0F);
    if (ImGui::InputText("Name", ws.trackNameBuf.data(), ws.trackNameBuf.size())) {
        track.name = ws.trackNameBuf.data();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(65.0F);
    int chanOneBased = track.channel + 1;
    if (ImGui::SliderInt("Ch", &chanOneBased, 1, 16)) {
        track.channel = std::clamp(chanOneBased - 1, 0, 15);
        if (track.channel == 9) {
            track.role = MIDI::MIDITrackRole::Percussion;
            track.isMainTrack = false;
        }
        RecomputeAllocation(ws, state);
    }
    ShowTextTooltip("MIDI Channel (1..16). Channel 10 is GM Percussion / SID Drum Kit.");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0F);
    static constexpr std::array<const char*, 5> ROLE_ITEMS = {
        "Auto", "* Main Melody", "Bass", "Harmony", "Percussion (Ch10)"
    };
    int roleIdx = static_cast<int>(track.role);
    if (track.isMainTrack) {
        roleIdx = 1;
    }
    if (ImGui::Combo("Role", &roleIdx, ROLE_ITEMS.data(), static_cast<int>(ROLE_ITEMS.size()))) {
        PushUndoState(ws);
        if (roleIdx == 1) {
            ws.project.SetMainTrack(ws.selectedTrackIndex);
        } else {
            if (track.isMainTrack) {
                track.isMainTrack = false;
            }
            track.role = static_cast<MIDI::MIDITrackRole>(roleIdx);
            if (track.role == MIDI::MIDITrackRole::Percussion) {
                track.channel = 9;
            }
        }
        RecomputeAllocation(ws, state);
    }

    const auto& presets = MIDI::MIDISoundBank::GetMelodicPresets();
    const auto& drumKits = MIDI::MIDISoundBank::GetPercussionKits();
    const auto& waveOpts = MIDI::MIDISoundBank::GetWaveformOptions();

    if (track.channel != 9) {
        std::string currentPresetLabel = MIDI::MIDISoundBank::GetProgramName(track.program);
        ImGui::SetNextItemWidth(220.0F);
        if (ImGui::BeginCombo("SID Preset", currentPresetLabel.c_str())) {
            for (const auto& preset : presets) {
                const bool selected = (track.program >= preset.programStart && track.program <= preset.programEnd);
                const std::string itemLabel =
                    std::format("[{:03d}-{:03d}] {} ({})", preset.programStart, preset.programEnd, preset.name,
                                MIDI::MIDISoundBank::FormatWaveformMask(preset.baseParams.waveMask));
                if (ImGui::Selectable(itemLabel.c_str(), selected)) {
                    PushUndoState(ws);
                    track.program = preset.representativeProgram;
                    ws.audioEngine.TriggerAuditionNote(state, track.channel, track.program, 60, 108,
                                                       track.customWaveOverride, GetEffectiveTrackRole(track));
                }
            }
            ImGui::EndCombo();
        }
    } else {
        DrawColoredText(ImVec4(0.94F, 0.38F, 0.57F, 1.0F), "Ch 10 Percussion Kit Audition:");
        static constexpr std::array<std::pair<const char*, int>, 6> DRUM_BUTTONS = {{
            {"Kick(36)", 36},
            {"Snare(38)", 38},
            {"CHat(42)", 42},
            {"OHat(46)", 46},
            {"Tom(45)", 45},
            {"Crash(49)", 49}
        }};
        for (std::size_t dIdx = 0; dIdx < DRUM_BUTTONS.size(); ++dIdx) {
            if (dIdx > 0) {
                ImGui::SameLine(0.0F, 4.0F);
            }
            if (ImGui::SmallButton(DRUM_BUTTONS.at(dIdx).first)) {
                ws.audioEngine.TriggerAuditionNote(state, 9, 0, DRUM_BUTTONS.at(dIdx).second, 115,
                                                   track.customWaveOverride, MIDI::MIDITrackRole::Percussion);
            }
        }
        if (!drumKits.empty() && ImGui::IsItemHovered()) {
            ShowTextTooltip("Click to audition SID noise/pitch-slide drum synthesis");
        }
    }

    std::string currentWaveLabel = "Auto (Instrument Default)";
    for (const auto& opt : waveOpts) {
        if (opt.waveMask == track.customWaveOverride) {
            currentWaveLabel = opt.label;
            break;
        }
    }
    ImGui::SetNextItemWidth(220.0F);
    if (ImGui::BeginCombo("Waveform", currentWaveLabel.c_str())) {
        for (const auto& opt : waveOpts) {
            const bool selected = (opt.waveMask == track.customWaveOverride);
            if (ImGui::Selectable(opt.label.c_str(), selected)) {
                PushUndoState(ws);
                track.customWaveOverride = opt.waveMask;
                const int previewPitch = (track.channel == 9) ? 38 : 60;
                ws.audioEngine.TriggerAuditionNote(state, track.channel, track.program, previewPitch, 108,
                                                   track.customWaveOverride, GetEffectiveTrackRole(track));
            }
        }
        ImGui::EndCombo();
    }
    ShowTextTooltip("Select Auto or override with single or mixed SID waveforms (e.g. SAW+PULSE, PULSE+TRI, SAW+TRI)");

    ImGui::SameLine();
    if (ImGui::SmallButton("Audition Patch")) {
        const int previewPitch = (track.channel == 9) ? 36 : 60;
        ws.audioEngine.TriggerAuditionNote(state, track.channel, track.program, previewPitch, 110,
                                           track.customWaveOverride, GetEffectiveTrackRole(track));
    }

    const int inspectPitch = (track.channel == 9) ? 38 : 60;
    const auto patch = MIDI::MIDISoundBank::GetInstrumentParams(
        track.program, track.channel, inspectPitch, 100, state.emulator.GetSID().GetModel(), track.customWaveOverride);

    const std::uint8_t attack = (patch.attackDecay >> 4U) & 0x0FU;
    const std::uint8_t decay = patch.attackDecay & 0x0FU;
    const std::uint8_t sustain = (patch.effectiveSustainRelease >> 4U) & 0x0FU;
    const std::uint8_t release = patch.effectiveSustainRelease & 0x0FU;
    const float dutyPct = (static_cast<float>(patch.pulseWidth & 0x0FFFU) / 4095.0F) * 100.0F;

    const std::string waveBadge = MIDI::MIDISoundBank::FormatWaveformMask(patch.waveMask);
    const std::string patchLine = std::format("Wave: [{}]  ADSR: {:X}/{:X}/{:X}/{:X}  PW: 0x{:03X} ({:.1f}%)",
                                              waveBadge, attack, decay, sustain, release, patch.pulseWidth, dutyPct);
    DrawColoredText(Theme::AccentActive, patchLine);

    const auto features = MIDI::MIDISoundBank::FormatFeatureList(patch.features);
    if (!features.empty()) {
        std::string featLine = "FX:";
        for (const auto& feat : features) {
            featLine += " [" + feat + "]";
        }
        DrawColoredText(Theme::TextSecondary, featLine);
    }

    ImGui::EndChild();
}

void DrawPianoRollToolbar(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (!ws.project.tracks.empty()) {
        if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
            ws.selectedTrackIndex = 0;
        }
        const auto& curTrack = ws.project.tracks.at(ws.selectedTrackIndex);
        ImGui::SetNextItemWidth(165.0F);
        if (ImGui::BeginCombo("##ActiveTrackCombo", curTrack.name.c_str())) {
            for (std::size_t idx = 0; idx < ws.project.tracks.size(); ++idx) {
                const auto& trk = ws.project.tracks.at(idx);
                const std::string item = std::format("{}{} (Ch{})", trk.isMainTrack ? "* " : "", trk.name, trk.channel + 1);
                if (ImGui::Selectable(item.c_str(), idx == ws.selectedTrackIndex)) {
                    ws.selectedTrackIndex = idx;
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Ghost Notes", &ws.showGhostNotes);

    ImGui::SameLine();
    ImGui::Checkbox("Color by SID Voice (V1/V2/V3)", &ws.colorBySidVoice);

    ImGui::SameLine();
    ImGui::TextUnformatted("Zoom H:");
    ImGui::SameLine(0.0F, 2.0F);
    if (ImGui::SmallButton("-##ZH")) {
        ws.pixelsPerBeat = std::clamp(ws.pixelsPerBeat - 15.0F, 30.0F, 240.0F);
    }
    ImGui::SameLine(0.0F, 2.0F);
    if (ImGui::SmallButton("+##ZH")) {
        ws.pixelsPerBeat = std::clamp(ws.pixelsPerBeat + 15.0F, 30.0F, 240.0F);
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Zoom V:");
    ImGui::SameLine(0.0F, 2.0F);
    if (ImGui::SmallButton("-##ZV")) {
        ws.rowHeight = std::clamp(ws.rowHeight - 2.0F, 12.0F, 28.0F);
    }
    ImGui::SameLine(0.0F, 2.0F);
    if (ImGui::SmallButton("+##ZV")) {
        ws.rowHeight = std::clamp(ws.rowHeight + 2.0F, 12.0F, 28.0F);
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Clear Track") && ws.selectedTrackIndex < ws.project.tracks.size()) {
        auto& trk = ws.project.tracks.at(ws.selectedTrackIndex);
        if (!trk.notes.empty()) {
            PushUndoState(ws);
            trk.notes.clear();
            RecomputeAllocation(ws, state);
            ws.statusLog = "Cleared all notes in active track.";
        }
    }

    int activeAtPlayhead = 0;
    int culledAtPlayhead = 0;
    const double ph = ws.audioEngine.GetPlayheadBeats();
    for (const auto& trk : ws.project.tracks) {
        if (trk.muted) {
            continue;
        }
        for (const auto& note : trk.notes) {
            if (ph >= note.startBeat && ph < (note.startBeat + note.durationBeats)) {
                ++activeAtPlayhead;
                if (note.assignedSidVoice < 0) {
                    ++culledAtPlayhead;
                }
            }
        }
    }

    ImGui::SameLine();
    const std::string polyBadge = std::format("SID Voices: {}/3", std::min(3, activeAtPlayhead));
    DrawColoredText(culledAtPlayhead > 0 ? Theme::RecActive : Theme::AccentActive, polyBadge);
    if (culledAtPlayhead > 0) {
        ImGui::SameLine();
        DrawColoredText(Theme::RecActive, std::format("({} culled)", culledAtPlayhead));
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();
    DrawColoredText(Theme::TextSecondary, ws.statusLog);
}

void DrawInteractivePianoRollAndVelocity(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (ws.project.tracks.empty()) {
        return;
    }
    if (ws.selectedTrackIndex >= ws.project.tracks.size()) {
        ws.selectedTrackIndex = 0;
    }
    auto& activeTrack = ws.project.tracks.at(ws.selectedTrackIndex);
    const auto activeRole = GetEffectiveTrackRole(activeTrack);

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 100.0F || avail.y < 100.0F) {
        return;
    }

    constexpr float KEYBOARD_WIDTH = 72.0F;
    constexpr float RULER_HEIGHT = 22.0F;
    constexpr float VELOCITY_HEIGHT = 68.0F;
    const float gridWidth = std::max(50.0F, avail.x - KEYBOARD_WIDTH);
    const float gridHeight = std::max(50.0F, avail.y - RULER_HEIGHT - VELOCITY_HEIGHT);

    if (!ws.initialScrollSet) {
        const int centerPitch = (activeTrack.channel == 9) ? 42 : 64;
        const int rowFromTop = MAX_PITCH - centerPitch;
        ws.scrollY = std::max(0.0F, (static_cast<float>(rowFromTop) * ws.rowHeight) - (gridHeight * 0.45F));
        ws.initialScrollSet = true;
    }

    const float maxScrollY = std::max(0.0F, (static_cast<float>(NUM_PITCHES) * ws.rowHeight) - gridHeight);
    ws.scrollY = std::clamp(ws.scrollY, 0.0F, maxScrollY);
    ws.scrollX = std::max(0.0F, ws.scrollX);

    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    const ImVec2 rulerMin(canvasOrigin.x + KEYBOARD_WIDTH, canvasOrigin.y);
    const ImVec2 rulerMax(canvasOrigin.x + avail.x, canvasOrigin.y + RULER_HEIGHT);
    const ImVec2 keysMin(canvasOrigin.x, canvasOrigin.y + RULER_HEIGHT);
    const ImVec2 keysMax(canvasOrigin.x + KEYBOARD_WIDTH, canvasOrigin.y + RULER_HEIGHT + gridHeight);
    const ImVec2 gridMin(canvasOrigin.x + KEYBOARD_WIDTH, canvasOrigin.y + RULER_HEIGHT);
    const ImVec2 gridMax(canvasOrigin.x + avail.x, canvasOrigin.y + RULER_HEIGHT + gridHeight);
    const ImVec2 velLabelMin(canvasOrigin.x, gridMax.y);
    const ImVec2 velLabelMax(canvasOrigin.x + KEYBOARD_WIDTH, canvasOrigin.y + avail.y);
    const ImVec2 velMin(canvasOrigin.x + KEYBOARD_WIDTH, gridMax.y);
    const ImVec2 velMax(canvasOrigin.x + avail.x, canvasOrigin.y + avail.y);

    ImGui::InvisibleButton("##PianoRollCanvas", avail,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    const bool canvasHovered = ImGui::IsItemHovered();
    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mousePos = io.MousePos;

    if (canvasHovered) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ws.scrollX = std::max(0.0F, ws.scrollX - io.MouseDelta.x);
            ws.scrollY = std::clamp(ws.scrollY - io.MouseDelta.y, 0.0F, maxScrollY);
        }
        if (std::abs(io.MouseWheel) > 0.0F) {
            if (io.KeyCtrl) {
                ws.pixelsPerBeat = std::clamp(ws.pixelsPerBeat + (io.MouseWheel * 12.0F), 30.0F, 240.0F);
            } else if (io.KeyShift) {
                ws.scrollX = std::max(0.0F, ws.scrollX - (io.MouseWheel * 48.0F));
            } else {
                ws.scrollY = std::clamp(ws.scrollY - (io.MouseWheel * 32.0F), 0.0F, maxScrollY);
            }
        }
    }

    const auto screenXToBeat = [&](float x) -> double {
        return std::max(0.0, static_cast<double>((x - gridMin.x + ws.scrollX) / ws.pixelsPerBeat));
    };
    const auto beatToScreenX = [&](double beat) -> float {
        return gridMin.x + static_cast<float>(beat * static_cast<double>(ws.pixelsPerBeat)) - ws.scrollX;
    };
    const auto screenYToPitch = [&](float y) -> int {
        const int rowFromTop = static_cast<int>(std::floor((y - gridMin.y + ws.scrollY) / ws.rowHeight));
        return std::clamp(MAX_PITCH - rowFromTop, MIN_PITCH, MAX_PITCH);
    };
    const auto pitchToScreenY = [&](int pitch) -> float {
        const int rowFromTop = MAX_PITCH - pitch;
        return gridMin.y + (static_cast<float>(rowFromTop) * ws.rowHeight) - ws.scrollY;
    };

    const bool inRuler =
        (mousePos.x >= rulerMin.x && mousePos.x <= rulerMax.x && mousePos.y >= rulerMin.y && mousePos.y <= rulerMax.y);
    const bool inKeys =
        (mousePos.x >= keysMin.x && mousePos.x <= keysMax.x && mousePos.y >= keysMin.y && mousePos.y <= keysMax.y);
    const bool inGrid =
        (mousePos.x >= gridMin.x && mousePos.x <= gridMax.x && mousePos.y >= gridMin.y && mousePos.y <= gridMax.y);
    const bool inVel =
        (mousePos.x >= velMin.x && mousePos.x <= velMax.x && mousePos.y >= velMin.y && mousePos.y <= velMax.y);

    if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (inRuler) {
            ws.dragMode = DragMode::ScrubTimeline;
            ws.audioEngine.SetPlayheadBeats(SnapBeatValue(screenXToBeat(mousePos.x), ws.snapBeats));
        } else if (inKeys) {
            ws.dragMode = DragMode::PianoKeyAudition;
            const int pitch = screenYToPitch(mousePos.y);
            ws.lastAuditionedPitch = pitch;
            ws.audioEngine.TriggerAuditionNote(state, activeTrack.channel, activeTrack.program, pitch, 108,
                                               activeTrack.customWaveOverride, activeRole);
        } else if (inVel) {
            ws.dragMode = DragMode::VelocityEdit;
            PushUndoState(ws);
        } else if (inGrid) {
            const double clickBeat = screenXToBeat(mousePos.x);
            const int clickPitch = screenYToPitch(mousePos.y);

            MIDI::MIDINote* hitNote = nullptr;
            bool hitRightEdge = false;
            for (auto& note : activeTrack.notes) {
                if (note.pitch != clickPitch) {
                    continue;
                }
                const float nx0 = beatToScreenX(note.startBeat);
                const float nx1 = beatToScreenX(note.startBeat + note.durationBeats);
                if (mousePos.x >= nx0 && mousePos.x <= nx1) {
                    hitNote = &note;
                    hitRightEdge = (nx1 - mousePos.x) <= 7.0F;
                    break;
                }
            }

            if (hitNote != nullptr) {
                PushUndoState(ws);
                if (!io.KeyShift) {
                    for (auto& n : activeTrack.notes) {
                        n.selected = false;
                    }
                }
                hitNote->selected = true;
                ws.activeNoteId = hitNote->id;
                ws.dragStartBeat = clickBeat;
                ws.dragStartPitch = clickPitch;
                ws.noteOrigStartBeat = hitNote->startBeat;
                ws.noteOrigDurationBeats = hitNote->durationBeats;
                ws.noteOrigPitch = hitNote->pitch;
                ws.lastAuditionedPitch = hitNote->pitch;

                if (hitRightEdge) {
                    ws.dragMode = DragMode::ResizeNote;
                } else {
                    ws.dragMode = DragMode::MoveNote;
                    ws.audioEngine.TriggerAuditionNote(state, activeTrack.channel, activeTrack.program, hitNote->pitch,
                                                       hitNote->velocity, activeTrack.customWaveOverride, activeRole);
                }
            } else if (io.KeyShift) {
                ws.dragMode = DragMode::BoxSelect;
                ws.boxSelectStart = mousePos;
                ws.boxSelectEnd = mousePos;
            } else {
                PushUndoState(ws);
                for (auto& n : activeTrack.notes) {
                    n.selected = false;
                }
                MIDI::MIDINote newNote;
                newNote.id = ws.project.nextNoteId++;
                newNote.pitch = clickPitch;
                newNote.velocity = 100;
                newNote.startBeat = FloorSnapBeatValue(clickBeat, ws.snapBeats);
                newNote.durationBeats = std::max(0.125, ws.defaultNoteDurationBeats);
                newNote.selected = true;

                ws.activeNoteId = newNote.id;
                ws.noteOrigStartBeat = newNote.startBeat;
                ws.noteOrigDurationBeats = newNote.durationBeats;
                ws.noteOrigPitch = newNote.pitch;
                ws.lastAuditionedPitch = newNote.pitch;
                activeTrack.notes.push_back(newNote);

                RecomputeAllocation(ws, state);
                ws.audioEngine.TriggerAuditionNote(state, activeTrack.channel, activeTrack.program, clickPitch, 100,
                                                   activeTrack.customWaveOverride, activeRole);
                ws.dragMode = DragMode::CreateNote;
                ws.statusLog = std::format("Placed note {} ({}) at beat {:.2f}", PitchToNoteName(clickPitch),
                                           clickPitch, newNote.startBeat);
            }
        }
    }

    if (canvasHovered && inGrid && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        const int clickPitch = screenYToPitch(mousePos.y);
        for (std::size_t idx = 0; idx < activeTrack.notes.size(); ++idx) {
            const auto& note = activeTrack.notes.at(idx);
            if (note.pitch != clickPitch) {
                continue;
            }
            const float nx0 = beatToScreenX(note.startBeat);
            const float nx1 = beatToScreenX(note.startBeat + note.durationBeats);
            if (mousePos.x >= nx0 && mousePos.x <= nx1) {
                PushUndoState(ws);
                activeTrack.notes.erase(activeTrack.notes.begin() + static_cast<std::ptrdiff_t>(idx));
                RecomputeAllocation(ws, state);
                ws.statusLog = "Deleted note.";
                break;
            }
        }
    }

    if (ws.dragMode != DragMode::None && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (ws.dragMode == DragMode::ScrubTimeline) {
            ws.audioEngine.SetPlayheadBeats(SnapBeatValue(screenXToBeat(mousePos.x), ws.snapBeats));
        } else if (ws.dragMode == DragMode::PianoKeyAudition) {
            const int pitch = screenYToPitch(mousePos.y);
            if (pitch != ws.lastAuditionedPitch) {
                ws.lastAuditionedPitch = pitch;
                ws.audioEngine.TriggerAuditionNote(state, activeTrack.channel, activeTrack.program, pitch, 108,
                                                   activeTrack.customWaveOverride, activeRole);
            }
        } else if (ws.dragMode == DragMode::CreateNote || ws.dragMode == DragMode::ResizeNote) {
            for (auto& note : activeTrack.notes) {
                if (note.id == ws.activeNoteId) {
                    const double endBeat = SnapBeatValue(screenXToBeat(mousePos.x), ws.snapBeats);
                    const double minStep = (ws.snapBeats > 0.0) ? ws.snapBeats : 0.125;
                    note.durationBeats = std::max(minStep, endBeat - note.startBeat);
                    ws.defaultNoteDurationBeats = note.durationBeats;
                    break;
                }
            }
        } else if (ws.dragMode == DragMode::MoveNote) {
            const double curBeat = screenXToBeat(mousePos.x);
            const int curPitch = screenYToPitch(mousePos.y);
            for (auto& note : activeTrack.notes) {
                if (note.id == ws.activeNoteId) {
                    const double deltaBeats = curBeat - ws.dragStartBeat;
                    note.startBeat = SnapBeatValue(ws.noteOrigStartBeat + deltaBeats, ws.snapBeats);
                    note.pitch = curPitch;
                    if (curPitch != ws.lastAuditionedPitch) {
                        ws.lastAuditionedPitch = curPitch;
                        ws.audioEngine.TriggerAuditionNote(state, activeTrack.channel, activeTrack.program, curPitch,
                                                           note.velocity, activeTrack.customWaveOverride, activeRole);
                    }
                    break;
                }
            }
        } else if (ws.dragMode == DragMode::BoxSelect) {
            ws.boxSelectEnd = mousePos;
            const float bx0 = std::min(ws.boxSelectStart.x, ws.boxSelectEnd.x);
            const float bx1 = std::max(ws.boxSelectStart.x, ws.boxSelectEnd.x);
            const float by0 = std::min(ws.boxSelectStart.y, ws.boxSelectEnd.y);
            const float by1 = std::max(ws.boxSelectStart.y, ws.boxSelectEnd.y);
            for (auto& note : activeTrack.notes) {
                const float nx0 = beatToScreenX(note.startBeat);
                const float nx1 = beatToScreenX(note.startBeat + note.durationBeats);
                const float ny0 = pitchToScreenY(note.pitch);
                const float ny1 = ny0 + ws.rowHeight;
                note.selected = (nx1 >= bx0 && nx0 <= bx1 && ny1 >= by0 && ny0 <= by1);
            }
        } else if (ws.dragMode == DragMode::VelocityEdit) {
            const float normY = std::clamp((velMax.y - mousePos.y) / (velMax.y - velMin.y - 6.0F), 0.0F, 1.0F);
            const int newVel = std::clamp(static_cast<int>(std::round(normY * 127.0F)), 1, 127);
            for (auto& note : activeTrack.notes) {
                const float nx0 = beatToScreenX(note.startBeat);
                const float nx1 = std::max(nx0 + 8.0F, beatToScreenX(note.startBeat + note.durationBeats));
                if (mousePos.x >= nx0 - 4.0F && mousePos.x <= nx1 + 4.0F) {
                    note.velocity = newVel;
                    const int sustainNib = std::clamp(4 + ((newVel * 11) / 127), 4, 15);
                    const int cutoffHz = 280 + ((newVel * 3200) / 127);
                    ImGui::BeginTooltip();
                    const std::string tip =
                        std::format("Velocity: {} | SID Sustain: 0x{:X} | Filter Cutoff: ~{} Hz", newVel, sustainNib,
                                    cutoffHz);
                    ImGui::TextUnformatted(tip.c_str());
                    ImGui::EndTooltip();
                }
            }
        }
    }

    if (ws.dragMode != DragMode::None && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (ws.dragMode == DragMode::CreateNote || ws.dragMode == DragMode::MoveNote ||
            ws.dragMode == DragMode::PianoKeyAudition) {
            ws.audioEngine.ReleaseAuditionNote(state);
        }
        if (ws.dragMode == DragMode::CreateNote || ws.dragMode == DragMode::MoveNote ||
            ws.dragMode == DragMode::ResizeNote) {
            RecomputeAllocation(ws, state);
        }
        ws.dragMode = DragMode::None;
        ws.lastAuditionedPitch = -1;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(canvasOrigin, ImVec2(canvasOrigin.x + KEYBOARD_WIDTH, canvasOrigin.y + RULER_HEIGHT),
                      ImColor(Theme::Surface));
    dl->AddText(ImVec2(canvasOrigin.x + 8.0F, canvasOrigin.y + 4.0F), ImColor(Theme::TextSecondary),
                activeTrack.channel == 9 ? "DRUMS" : "KEYS");

    dl->PushClipRect(gridMin, gridMax, true);
    for (int pitch = MIN_PITCH; pitch <= MAX_PITCH; ++pitch) {
        const float y0 = pitchToScreenY(pitch);
        const float y1 = y0 + ws.rowHeight;
        if (y1 < gridMin.y || y0 > gridMax.y) {
            continue;
        }
        const bool black = IsBlackKey(pitch);
        const ImU32 rowBg = black ? IM_COL32(13, 16, 23, 255) : IM_COL32(20, 24, 34, 255);
        dl->AddRectFilled(ImVec2(gridMin.x, y0), ImVec2(gridMax.x, y1), rowBg);
        const ImU32 lineCol = (pitch % 12 == 0) ? IM_COL32(60, 75, 100, 200) : IM_COL32(32, 38, 52, 160);
        dl->AddLine(ImVec2(gridMin.x, y1), ImVec2(gridMax.x, y1), lineCol);
    }

    const double startBeatVis = std::floor(screenXToBeat(gridMin.x));
    const double endBeatVis = std::ceil(screenXToBeat(gridMax.x)) + 1.0;
    const double gridStep = (ws.snapBeats > 0.0 && ws.pixelsPerBeat * static_cast<float>(ws.snapBeats) >= 10.0F)
                                ? ws.snapBeats
                                : 1.0;

    for (double beat = std::max(0.0, startBeatVis); beat <= endBeatVis; beat += gridStep) {
        const float x = beatToScreenX(beat);
        if (x < gridMin.x || x > gridMax.x) {
            continue;
        }
        const double nearestInt = std::round(beat);
        const bool isBeatLine = std::abs(beat - nearestInt) < 1e-4;
        const bool isBarLine = isBeatLine && (static_cast<int>(nearestInt) % ws.project.timeSigNumerator == 0);
        const ImU32 vCol = isBarLine    ? IM_COL32(85, 105, 140, 220)
                           : isBeatLine ? IM_COL32(48, 58, 78, 180)
                                        : IM_COL32(30, 36, 48, 120);
        dl->AddLine(ImVec2(x, gridMin.y), ImVec2(x, gridMax.y), vCol);
    }

    if (ws.showGhostNotes) {
        for (std::size_t tIdx = 0; tIdx < ws.project.tracks.size(); ++tIdx) {
            if (tIdx == ws.selectedTrackIndex) {
                continue;
            }
            const auto& other = ws.project.tracks.at(tIdx);
            if (other.muted) {
                continue;
            }
            for (const auto& note : other.notes) {
                const float x0 = beatToScreenX(note.startBeat);
                const float x1 = std::max(x0 + 4.0F, beatToScreenX(note.startBeat + note.durationBeats));
                const float y0 = pitchToScreenY(note.pitch) + 2.0F;
                const float y1 = y0 + ws.rowHeight - 4.0F;
                if (x1 < gridMin.x || x0 > gridMax.x || y1 < gridMin.y || y0 > gridMax.y) {
                    continue;
                }
                dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(120, 140, 175, 55), 2.0F);
            }
        }
    }

    for (const auto& note : activeTrack.notes) {
        const float x0 = beatToScreenX(note.startBeat);
        const float x1 = std::max(x0 + 6.0F, beatToScreenX(note.startBeat + note.durationBeats));
        const float y0 = pitchToScreenY(note.pitch) + 1.0F;
        const float y1 = y0 + ws.rowHeight - 2.0F;
        if (x1 < gridMin.x || x0 > gridMax.x || y1 < gridMin.y || y0 > gridMax.y) {
            continue;
        }

        ImU32 fillCol = IM_COL32(79, 195, 247, 230);
        if (ws.colorBySidVoice) {
            if (note.assignedSidVoice >= 0 && note.assignedSidVoice < 3) {
                fillCol = SID_VOICE_COLORS.at(static_cast<std::size_t>(note.assignedSidVoice));
            } else {
                fillCol = SID_CULLED_COLOR;
            }
        } else {
            fillCol = ImColor(GetRoleBadgeColor(activeRole));
        }

        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), fillCol, 3.0F);

        const ImU32 borderCol = note.selected              ? IM_COL32(255, 255, 255, 255)
                                : activeTrack.isMainTrack ? IM_COL32(255, 215, 64, 220)
                                                          : IM_COL32(15, 20, 28, 220);
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), borderCol, 3.0F, 0, note.selected ? 2.0F : 1.0F);

        if ((x1 - x0) > 26.0F && ws.rowHeight >= 13.0F) {
            const std::string lbl = (activeTrack.channel == 9)
                                        ? MIDI::MIDISoundBank::GetPercussionNoteName(note.pitch)
                                        : PitchToNoteName(note.pitch);
            dl->AddText(ImVec2(x0 + 4.0F, y0 + 1.0F), IM_COL32(12, 16, 22, 245), lbl.c_str());
        }
    }

    if (ws.dragMode == DragMode::BoxSelect) {
        const ImVec2 bMin(std::min(ws.boxSelectStart.x, ws.boxSelectEnd.x),
                          std::min(ws.boxSelectStart.y, ws.boxSelectEnd.y));
        const ImVec2 bMax(std::max(ws.boxSelectStart.x, ws.boxSelectEnd.x),
                          std::max(ws.boxSelectStart.y, ws.boxSelectEnd.y));
        dl->AddRectFilled(bMin, bMax, IM_COL32(79, 195, 247, 45));
        dl->AddRect(bMin, bMax, IM_COL32(79, 195, 247, 210));
    }

    const float playheadX = beatToScreenX(ws.audioEngine.GetPlayheadBeats());
    if (playheadX >= gridMin.x && playheadX <= gridMax.x) {
        dl->AddLine(ImVec2(playheadX, gridMin.y), ImVec2(playheadX, gridMax.y), IM_COL32(16, 235, 160, 240), 2.0F);
    }
    dl->PopClipRect();

    dl->PushClipRect(rulerMin, rulerMax, true);
    dl->AddRectFilled(rulerMin, rulerMax, ImColor(Theme::Surface));
    for (int beatInt = static_cast<int>( std::max(0.0, startBeatVis)); beatInt <= static_cast<int>(endBeatVis);
         ++beatInt) {
        const float x = beatToScreenX(static_cast<double>(beatInt));
        if (x < rulerMin.x || x > rulerMax.x) {
            continue;
        }
        const bool isBar = (beatInt % ws.project.timeSigNumerator == 0);
        dl->AddLine(ImVec2(x, isBar ? rulerMin.y : rulerMin.y + 12.0F), ImVec2(x, rulerMax.y),
                    IM_COL32(120, 140, 170, 200));
        if (isBar) {
            const int barNum = (beatInt / ws.project.timeSigNumerator) + 1;
            const std::string barTxt = std::format("Bar {}", barNum);
            dl->AddText(ImVec2(x + 4.0F, rulerMin.y + 3.0F), ImColor(Theme::TextMain), barTxt.c_str());
        }
    }
    if (playheadX >= rulerMin.x && playheadX <= rulerMax.x) {
        dl->AddTriangleFilled(ImVec2(playheadX - 6.0F, rulerMin.y + 2.0F), ImVec2(playheadX + 6.0F, rulerMin.y + 2.0F),
                              ImVec2(playheadX, rulerMax.y - 1.0F), IM_COL32(16, 235, 160, 255));
    }
    dl->PopClipRect();

    dl->PushClipRect(keysMin, keysMax, true);
    for (int pitch = MIN_PITCH; pitch <= MAX_PITCH; ++pitch) {
        const float y0 = pitchToScreenY(pitch);
        const float y1 = y0 + ws.rowHeight;
        if (y1 < keysMin.y || y0 > keysMax.y) {
            continue;
        }
        const bool black = IsBlackKey(pitch);
        const bool isAuditioned = (ws.lastAuditionedPitch == pitch);
        const ImU32 keyBg = isAuditioned ? IM_COL32(16, 185, 129, 255)
                            : black      ? IM_COL32(28, 32, 44, 255)
                                         : IM_COL32(215, 222, 235, 255);
        const ImU32 txtCol = (black || isAuditioned) ? IM_COL32(235, 240, 250, 255) : IM_COL32(20, 25, 35, 255);
        dl->AddRectFilled(ImVec2(keysMin.x, y0), ImVec2(keysMax.x, y1 - 1.0F), keyBg);

        if (activeTrack.channel == 9) {
            const std::string drumName =
                std::format("{} {}", pitch, MIDI::MIDISoundBank::GetPercussionNoteName(pitch));
            dl->AddText(ImVec2(keysMin.x + 3.0F, y0 + 1.0F), txtCol, drumName.c_str());
        } else if (pitch % 12 == 0 || isAuditioned) {
            const std::string noteName = PitchToNoteName(pitch);
            dl->AddText(ImVec2(keysMin.x + 6.0F, y0 + 1.0F), txtCol, noteName.c_str());
        }
    }
    dl->PopClipRect();

    dl->AddRectFilled(velLabelMin, velLabelMax, ImColor(Theme::Surface));
    dl->AddText(ImVec2(velLabelMin.x + 6.0F, velLabelMin.y + 8.0F), ImColor(Theme::AccentPrimary), "VELOCITY");
    dl->AddText(ImVec2(velLabelMin.x + 6.0F, velLabelMin.y + 24.0F), ImColor(Theme::TextSecondary), "Sustain/");
    dl->AddText(ImVec2(velLabelMin.x + 6.0F, velLabelMin.y + 38.0F), ImColor(Theme::TextSecondary), "Cutoff");

    dl->PushClipRect(velMin, velMax, true);
    dl->AddRectFilled(velMin, velMax, IM_COL32(12, 15, 22, 255));
    dl->AddLine(velMin, ImVec2(velMax.x, velMin.y), ImColor(Theme::BorderSubtle), 1.5F);
    for (const auto& note : activeTrack.notes) {
        const float x = beatToScreenX(note.startBeat);
        if (x < velMin.x - 10.0F || x > velMax.x + 10.0F) {
            continue;
        }
        const float ratio = std::clamp(static_cast<float>(note.velocity) / 127.0F, 0.05F, 1.0F);
        const float barH = ratio * (VELOCITY_HEIGHT - 10.0F);
        const ImVec2 b0(x, velMax.y - barH);
        const ImVec2 b1(x + 6.0F, velMax.y - 2.0F);
        const ImU32 barCol = note.selected ? IM_COL32(255, 230, 90, 255) : IM_COL32(16, 185, 129, 220);
        dl->AddRectFilled(b0, b1, barCol, 2.0F);
    }
    if (playheadX >= velMin.x && playheadX <= velMax.x) {
        dl->AddLine(ImVec2(playheadX, velMin.y), ImVec2(playheadX, velMax.y), IM_COL32(16, 235, 160, 200), 1.5F);
    }
    dl->PopClipRect();
}

void HandleMIDIWorkspaceFileDialogs(MIDIWorkspaceState& ws, Control::AppState& state) {
    if (Frontend::CustomFileDialog::Display("MIDI_OpenFile")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            const std::string path = Frontend::CustomFileDialog::GetFilePathName();
            OpenMIDIFileInWorkspace(state, path);
        }
        Frontend::CustomFileDialog::Close();
    }

    if (Frontend::CustomFileDialog::Display("MIDI_SaveFile")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            std::string path = Frontend::CustomFileDialog::GetFilePathName();
            if (!path.ends_with(".mid") && !path.ends_with(".midi")) {
                path += ".mid";
            }
            std::string err;
            if (MIDI::MIDIFile::SaveToFile(path, ws.project, err)) {
                ws.project.filePath = path;
                ws.statusLog = "Saved MIDI to " + path;
            } else {
                ws.statusLog = "Failed to save MIDI: " + err;
            }
        }
        Frontend::CustomFileDialog::Close();
    }

#ifndef TARGET_WASM
    if (Frontend::CustomFileDialog::Display("MIDI_ExportBinSid")) {
        if (Frontend::CustomFileDialog::IsOk()) {
            const std::string targetPath = Frontend::CustomFileDialog::GetFilePathName();
            const bool wantSid = targetPath.ends_with(".sid");
            std::vector<std::uint8_t> compiledBytes;
            std::string builtPath;
            if (CompileMIDIProjectToBinary(ws, state, wantSid, compiledBytes, builtPath)) {
                std::ofstream out(targetPath, std::ios::binary);
                if (out) {
                    for (const auto byteVal : compiledBytes) {
                        out.put(static_cast<char>(byteVal));
                    }
                    ws.statusLog = std::format("Exported {} bytes to {}", compiledBytes.size(), targetPath);
                } else {
                    ws.statusLog = "Failed to write export file: " + targetPath;
                }
            }
        }
        Frontend::CustomFileDialog::Close();
    }
#endif
}

void HandleKeyboardShortcuts(MIDIWorkspaceState& ws, Control::AppState& state) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (ws.audioEngine.IsPlaying()) {
            ws.audioEngine.Pause(state);
        } else {
            ws.audioEngine.Play(state, ws.project);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
        DeleteSelectedNotes(ws, state);
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        PerformUndo(ws, state);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        PerformRedo(ws, state);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        CopySelectedNotes(ws);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        PasteNotes(ws, state);
    }
}

}  // namespace

void OpenMIDIFileInWorkspace(Control::AppState& state, const std::string& filePath) {
    auto& ws = GetWorkspaceState();
    EnsureWorkspaceInitialized(ws, state);
    ws.audioEngine.Stop(state);

    MIDI::MIDIProject loaded;
    std::string err;
    if (MIDI::MIDIFile::LoadFromFile(filePath, loaded, err)) {
        PushUndoState(ws);
        ws.project = std::move(loaded);
        ws.selectedTrackIndex = 0;
        for (std::size_t idx = 0; idx < ws.project.tracks.size(); ++idx) {
            if (ws.project.tracks.at(idx).isMainTrack) {
                ws.selectedTrackIndex = idx;
                break;
            }
        }
        ws.lastSyncedTrackIdx = 999999;
        ws.initialScrollSet = false;
        RecomputeAllocation(ws, state);
        state.ide.currentFilePath = filePath;
        ws.statusLog = std::format("Loaded '{}' ({} tracks, {:.0f} BPM)",
                                   std::filesystem::path(filePath).filename().string(), ws.project.tracks.size(),
                                   ws.project.bpm);
    } else {
        ws.statusLog = "Failed to load MIDI: " + err;
    }
}

bool IsMIDIAudioActive() {
    return GetWorkspaceState().audioEngine.IsActive();
}

void DrawMIDIWorkspaceWindow(Control::AppState& state, ImVec2 workPos, ImVec2 workSize, float topSectionHeight,
                             ImGuiWindowFlags windowFlags) {
    auto& ws = GetWorkspaceState();
    EnsureWorkspaceInitialized(ws, state);

    ws.audioEngine.Update(state, ws.project, ImGui::GetIO().DeltaTime);
    HandleKeyboardShortcuts(ws, state);

    const float leftWidth = workSize.x * 0.47F;
    ImGui::SetNextWindowPos(workPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, topSectionHeight), ImGuiCond_Always);
    if (ImGui::Begin("65C02 MIDI Studio — Tracks & Instruments", nullptr, windowFlags | ImGuiWindowFlags_NoMove)) {
        DrawTopRowModeAndFileBar(ws, state);
        ImGui::Separator();
        DrawSecondRowTransportAndPipelineBar(ws, state);
        ImGui::Separator();

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float trackListW = avail.x * 0.44F;
        const float inspectorW = avail.x - trackListW - ImGui::GetStyle().ItemSpacing.x;
        DrawTrackListPanel(ws, state, trackListW, avail.y);
        ImGui::SameLine();
        DrawTrackInspectorPanel(ws, state, inspectorW, avail.y);
    }
    ImGui::End();

    const float bottomY = workPos.y + topSectionHeight;
    const float bottomH = workSize.y - topSectionHeight;
    ImGui::SetNextWindowPos(ImVec2(workPos.x, bottomY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(workSize.x, bottomH), ImGuiCond_Always);
    if (ImGui::Begin("Piano Roll & Velocity Editor", nullptr, windowFlags | ImGuiWindowFlags_NoMove)) {
        DrawPianoRollToolbar(ws, state);
        ImGui::Separator();
        DrawInteractivePianoRollAndVelocity(ws, state);
    }
    ImGui::End();

    HandleMIDIWorkspaceFileDialogs(ws, state);
}

}  // namespace GUI
