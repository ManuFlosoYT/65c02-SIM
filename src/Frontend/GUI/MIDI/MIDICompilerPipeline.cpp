#include "Frontend/GUI/MIDI/MIDICompilerPipeline.h"
#include "Frontend/GUI/MIDI/MIDIProjectSession.h"
#include "Frontend/GUI/MIDI/MIDIAudioEngine.h"
#include "Frontend/Control/AppState.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace GUI::MIDI {

std::string MIDICompilerPipeline::SanitizeCleanStem(const std::string& path) {
    std::string stem = std::filesystem::path(path).stem().string();
    std::erase(stem, ' ');
    if (stem.empty()) {
        stem = "ide_preview";
    }
    return stem;
}

#ifndef TARGET_WASM
namespace {

std::vector<std::uint8_t> ReadBinaryFileBytes(const std::string& path) {
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

std::string ReadTextFileContent(const std::string& path) {
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile) {
        return {};
    }
    return {std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>()};
}

} // namespace

bool MIDICompilerPipeline::CompileMIDIProjectToBinary(
    MIDIProjectSession& session,
    Control::AppState& state,
    bool isNtsc,
    bool microDosSid,
    std::vector<std::uint8_t>& outBinary,
    std::string& outSavedPath) {

    auto& project = session.GetProject();
    std::error_code ec;
    std::filesystem::create_directories("tools/sid/build", ec);
    std::filesystem::create_directories("output/midi", ec);

    std::string midPath = project.filePath;
    if (midPath.empty()) {
        midPath = "tools/sid/build/ide_preview.mid";
    }
    std::string saveErr;
    if (!MIDIFile::SaveToFile(midPath, project, saveErr)) {
        session.SetStatusLog("Failed to save temporary MIDI: " + saveErr);
        return false;
    }

    const std::string cleanName = SanitizeCleanStem(midPath);
    const std::string chipStr = (state.emulator.GetSID().GetModel() == Hardware::SIDModel::MOS6581) ? "6581" : "8580";
    const std::string sysStr = isNtsc ? "NTSC" : "PAL";
    const int mainTrackIdx = project.GetMainTrackIndex();

    std::string extraArgs = std::format("--chip {} --system {}", chipStr, sysStr);
    if (mainTrackIdx >= 0 && static_cast<std::size_t>(mainTrackIdx) < project.tracks.size()) {
        const auto& mainTrk = project.tracks.at(static_cast<std::size_t>(mainTrackIdx));
        extraArgs += std::format(" --main-track {} --melody-channel {}", mainTrackIdx + 1, mainTrk.channel);
    }
    for (const auto& trk : project.tracks) {
        if (trk.role == MIDITrackRole::Bass && trk.channel != 9) {
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
            session.SetStatusLog(std::format("Built MicroDOS .sid ({} bytes): {}", outBinary.size(), outSidPath));
            return !outBinary.empty();
        }
        session.SetStatusLog("audio_to_sid.py (--microDOS) failed. Check console/output.");
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
            session.SetStatusLog(std::format("midi-to-bin.sh succeeded ({} bytes): {}", outBinary.size(), outBinPath));
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
            session.SetStatusLog(std::format("Compiled SID ROM [mode: {}] ({} bytes).", mode, outBinary.size()));
            return true;
        }
    }

    session.SetStatusLog("Failed to compile MIDI to 65C02 ROM.");
    return false;
}

bool MIDICompilerPipeline::HandleCompileAndRunROM(
    MIDIProjectSession& session,
    MIDIAudioEngine& audioEngine,
    Control::AppState& state) {

    audioEngine.Stop(state);
    std::vector<std::uint8_t> binary;
    std::string binPath;
    if (!CompileMIDIProjectToBinary(session, state, audioEngine.IsNTSC(), false, binary, binPath)) {
        return false;
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
        session.SetStatusLog("Loaded & running on 65C02 Emulator: " + romName);
        return true;
    }

    session.SetStatusLog("Emulator load failed: " + errMsg);
    return false;
}
#endif

} // namespace GUI::MIDI
