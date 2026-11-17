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
#include "Frontend/Control/SDKInstaller.h"
#include "Frontend/Platform/PlatformUtils.h"
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

class ScopedCurrentPath {
public:
    explicit ScopedCurrentPath(const std::filesystem::path& targetDir) {
        std::error_code ec;
        const std::filesystem::path current = std::filesystem::current_path(ec);
        if (!ec && !targetDir.empty() && current.lexically_normal() != targetDir.lexically_normal()) {
            previousPath_ = current;
            std::filesystem::current_path(targetDir, ec);
            changed_ = !ec;
        }
    }

    ~ScopedCurrentPath() {
        if (changed_) {
            std::error_code ec;
            std::filesystem::current_path(previousPath_, ec);
        }
    }

    ScopedCurrentPath(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath(ScopedCurrentPath&&) = delete;
    ScopedCurrentPath& operator=(ScopedCurrentPath&&) = delete;

private:
    std::filesystem::path previousPath_;
    bool changed_ = false;
};

bool IsWritableDirectory(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        return false;
    }
    const std::filesystem::path probe = dir / ".write_probe";
    {
        std::ofstream testOut(probe, std::ios::binary | std::ios::trunc);
        if (!testOut.is_open()) {
            return false;
        }
    }
    std::filesystem::remove(probe, ec);
    return true;
}

std::string PublishArtifact(const std::filesystem::path& src, const std::filesystem::path& dst) {
    std::error_code ec;
    if (src == dst) {
        return dst.string();
    }
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
    return (!ec && std::filesystem::exists(dst, ec)) ? dst.string() : src.string();
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
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    const std::filesystem::path appDataDir = Frontend::Platform::GetAppDataDirectory();
    const std::filesystem::path sdkDir = Control::SDKInstaller::GetSDKDir();

    const bool localToolsExist = std::filesystem::exists("tools/sid/audio_to_sid.py", ec);
    const bool localWritable =
        localToolsExist && IsWritableDirectory("tools/sid/build") && IsWritableDirectory("output/midi");
    const bool sdkToolsExist = std::filesystem::exists(sdkDir / "tools/sid/audio_to_sid.py", ec);

    std::filesystem::path toolRoot = cwd;
    std::filesystem::path baseDir = appDataDir;
    std::filesystem::path audioToSidPath = "tools/sid/audio_to_sid.py";
    std::filesystem::path midiToBinPath = "sdk/scripts/midi-to-bin.sh";

    if (localWritable) {
        baseDir = cwd;
    } else if (sdkToolsExist) {
        toolRoot = sdkDir;
        audioToSidPath = sdkDir / "tools/sid/audio_to_sid.py";
        if (std::filesystem::exists(sdkDir / "sdk/scripts/midi-to-bin.sh", ec)) {
            midiToBinPath = sdkDir / "sdk/scripts/midi-to-bin.sh";
        }
    } else if (localToolsExist) {
        audioToSidPath = cwd / "tools/sid/audio_to_sid.py";
        if (std::filesystem::exists(cwd / "sdk/scripts/midi-to-bin.sh", ec)) {
            midiToBinPath = cwd / "sdk/scripts/midi-to-bin.sh";
        }
    }

    const std::filesystem::path buildDir = baseDir / "tools/sid/build";
    const std::filesystem::path outputMidiDir = baseDir / "output/midi";
    std::filesystem::create_directories(buildDir, ec);
    std::filesystem::create_directories(outputMidiDir, ec);

    std::string midPath = project.filePath;
    if (midPath.empty()) {
        midPath = (buildDir / "ide_preview.mid").string();
    }
    std::string saveErr;
    if (!MIDIFile::SaveToFile(midPath, project, saveErr)) {
        session.SetStatusLog("Failed to save temporary MIDI: " + saveErr);
        return false;
    }
    std::error_code absEc;
    const std::filesystem::path absMidPath = std::filesystem::absolute(midPath, absEc);
    const std::string effectiveMidPath = (!absEc && !absMidPath.empty()) ? absMidPath.string() : midPath;

    const std::string cleanName = SanitizeCleanStem(midPath);
    const std::string chipStr = (state.backend && state.backend->GetSID().GetModel() == Hardware::SIDModel::MOS6581) ? "6581" : "8580";
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
        const std::filesystem::path toolSidPath = toolRoot / "output/midi" / (cleanName + ".sid");
        const std::filesystem::path outSidPath = outputMidiDir / (cleanName + ".sid");
        std::filesystem::remove(toolSidPath, ec);
        std::filesystem::remove(outSidPath, ec);
        std::string cmdOut;
        const std::string cmd = std::format(
            "python3 \"{}\" \"{}\" --mode max_quality --microDOS {}",
            audioToSidPath.string(),
            effectiveMidPath,
            extraArgs);
        int rc = 0;
        {
            const ScopedCurrentPath cwdScope(toolRoot);
            rc = CompilerFrontend::RunCommand(cmd, cmdOut);
        }
        if (rc == 0 && std::filesystem::exists(toolSidPath, ec)) {
            outBinary = ReadBinaryFileBytes(toolSidPath.string());
            if (!outBinary.empty()) {
                outSavedPath = PublishArtifact(toolSidPath, outSidPath);
                session.SetStatusLog(std::format("Built MicroDOS .sid ({} bytes): {}", outBinary.size(), outSavedPath));
                return true;
            }
        }
        session.SetStatusLog("audio_to_sid.py (--microDOS) failed. Check console/output.");
        state.ide.outputLog = cmdOut;
        return false;
    }

    const std::filesystem::path toolBinPath = toolRoot / "output/midi" / (cleanName + ".bin");
    const std::filesystem::path outBinPath = outputMidiDir / (cleanName + ".bin");
    std::filesystem::remove(toolBinPath, ec);
    std::filesystem::remove(outBinPath, ec);

    std::string scriptOut;
    const std::string scriptCmd =
        std::format("bash \"{}\" \"{}\" {}", midiToBinPath.string(), effectiveMidPath, extraArgs);
    int scriptRc = 0;
    {
        const ScopedCurrentPath cwdScope(toolRoot);
        scriptRc = CompilerFrontend::RunCommand(scriptCmd, scriptOut);
    }
    state.ide.outputLog = scriptOut;

    if (scriptRc == 0 && std::filesystem::exists(toolBinPath, ec)) {
        outBinary = ReadBinaryFileBytes(toolBinPath.string());
        if (!outBinary.empty()) {
            outSavedPath = PublishArtifact(toolBinPath, outBinPath);
            session.SetStatusLog(std::format("midi-to-bin.sh succeeded ({} bytes): {}", outBinary.size(), outSavedPath));
            return true;
        }
    }

    static constexpr std::array<const char*, 5> FALLBACK_MODES = {
        "max_quality", "quality", "balanced", "compressed", "max_compression"
    };
    const std::filesystem::path asmPath = toolRoot / "tools/sid/build" / (cleanName + ".s");
    for (const char* mode : FALLBACK_MODES) {
        std::filesystem::remove(asmPath, ec);
        std::string pyOut;
        const std::string pyCmd = std::format(
            "python3 \"{}\" \"{}\" --mode {} {}",
            audioToSidPath.string(),
            effectiveMidPath,
            mode,
            extraArgs);
        int pyRc = 0;
        {
            const ScopedCurrentPath cwdScope(toolRoot);
            pyRc = CompilerFrontend::RunCommand(pyCmd, pyOut);
        }
        state.ide.outputLog += pyOut;
        if (pyRc != 0 || !std::filesystem::exists(asmPath, ec)) {
            continue;
        }
        const std::string asmCode = ReadTextFileContent(asmPath.string());
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
            outSavedPath = outBinPath.string();
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

    if (!state.backend) {
        state.CreateBackend(state.cachedSidSampleRate);
    }
    if (!state.backend) {
        session.SetStatusLog("Failed to create emulator backend.");
        return false;
    }
    state.backend->Pause();
    state.backend->SetCartridge(Core::Cartridge{});
    state.backend->GetSID().EnableSound(true);
    state.backend->SetSIDEnabled(true);

    std::string errMsg;
    const std::string romName = std::filesystem::path(binPath).filename().string();
    if (state.backend->InitFromMemory(binary, romName, errMsg)) {
        state.rom.bin = romName;
        state.rom.data = binary;
        state.rom.loaded = true;
        state.backend->ClearProfiler();
        state.backend->Resume();
        state.ide.open = false;
        session.SetStatusLog("Loaded & running on 65C02 Emulator: " + romName);
        return true;
    }

    session.SetStatusLog("Emulator load failed: " + errMsg);
    return false;
}
#endif

} // namespace GUI::MIDI
