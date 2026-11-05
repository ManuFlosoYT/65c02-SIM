#include "Frontend/HeadlessRunner.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include "Hardware/Core/Logger.h"

#include "Hardware/Audio/NSFRunner.h"
#include "Hardware/Core/Emulator/Emulator.h"

namespace Frontend {

namespace {

std::string ResolveHeadlessRomPath(const Args& args) {
    if (!args.romPath.empty()) {
        return args.romPath;
    }

    static constexpr const char* fallbackCandidates[] = {
        "output/rom/room.bin",
        "./output/rom/room.bin",
        "room.bin"
    };

    for (const char* candidate : fallbackCandidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

}  // namespace

int HeadlessRunner::Run(const Args& args, int sidSampleRate) {
    if (!args.nsfTracePath.empty()) {
        if (args.romPath.empty()) {
            Core::Logger::Error("[NSFRunner] Error: No input .nsf path specified.");
            return 1;
        }
        const Hardware::NSFRunConfig nsfConfig{
            .maxDurationSec = args.nsfMaxDuration,
            .songIndex = args.nsfSong,
        };
        const Hardware::NSFRunResult res = Hardware::NSFRunner::RunFile(args.romPath, nsfConfig);
        if (!res.success) {
            Core::Logger::Error("[NSFRunner] Error: {}", res.errorMessage);
            return 1;
        }
        if (!Hardware::NSFRunner::WriteTraceJson(res, args.nsfTracePath)) {
            Core::Logger::Error("[NSFRunner] Error: Failed to write trace JSON to {}", args.nsfTracePath);
            return 1;
        }
        Core::Logger::Info("[NSFRunner] Trace written to {} ({} frames, {:.2f}s, stop={})",
                           args.nsfTracePath, res.totalFrames, res.durationSec, res.stopReason);
        return 0;
    }

    Core::Emulator emulator;
    emulator.SetRewindSnapshotsEnabled(false);
    emulator.GetSID().Init(sidSampleRate);
    emulator.SetSIDEnabled(args.sidDevice);
    emulator.GetSID().EnableSound(args.sidSound);
    emulator.GetSID().SetProfilingEnabled(args.profileSID);
    emulator.GetSID().ClearProfilingStats();
    emulator.SetGPUEnabled(false);

    std::string romPath = ResolveHeadlessRomPath(args);
    if (romPath.empty()) {
        Core::Logger::Error("Headless: No ROM provided and fallback ROM not found.\nTried: output/rom/room.bin, ./output/rom/room.bin, room.bin");
        return -1;
    }
    if (args.romPath.empty()) {
        Core::Logger::Warn("Headless: No ROM specified, using fallback ROM: {}", romPath);
    }

    std::string errorMsg;
    if (!emulator.Init(romPath, errorMsg)) {
        Core::Logger::Error("Headless: Failed to load ROM: {}", errorMsg);
        return -1;
    }

    if (!args.scriptPath.empty()) {
        emulator.GetScriptEngine().SetMirrorToStdout(true);
        emulator.GetScriptEngine().LoadAndRun(args.scriptPath);
    }

    if (emulator.GetScriptEngine().HasPendingScript()) {
        emulator.GetScriptEngine().ExecutePendingScript();
    }

    if (args.runCycles > 0) {
        emulator.Resume();
        for (int i = 0; i < args.runCycles; ++i) {
            if (emulator.IsHalted() || emulator.Step() != 0) {
                break;
            }
        }
        if (!emulator.IsPaused()) {
            emulator.Pause();
        }
    }

    if (args.profileSID) {
        const auto stats = emulator.GetSID().GetProfilingStats();
        const auto toMilliseconds = [](std::uint64_t nanoseconds) {
            return static_cast<double>(nanoseconds) / 1'000'000.0;
        };
        Core::Logger::Info("SID profile: clocks={}, oscillator_ms={:.3f}, envelope_ms={:.3f}, filter_ms={:.3f}, resampler_ms={:.3f}, output_ms={:.3f}",
                           stats.clockCount,
                           toMilliseconds(stats.oscillatorNanoseconds),
                           toMilliseconds(stats.envelopeNanoseconds),
                           toMilliseconds(stats.filterNanoseconds),
                           toMilliseconds(stats.resamplerNanoseconds),
                           toMilliseconds(stats.outputNanoseconds));
    }

    if (!args.dumpMemPath.empty()) {
        std::ofstream out(args.dumpMemPath, std::ios::binary);
        if (out.is_open()) {
            auto& mem = emulator.GetMem();
            for (uint32_t addr = 0; addr <= 0xFFFF; ++addr) {
                char val = static_cast<char>(mem.Read<true>(static_cast<uint16_t>(addr)));
                out.write(&val, 1);
            }
        } else {
            Core::Logger::Error("Headless: Failed to open dump file {}", args.dumpMemPath);
        }
    }

    emulator.Stop();
    emulator.GetSID().Close();
    return 0;
}

}  // namespace Frontend
