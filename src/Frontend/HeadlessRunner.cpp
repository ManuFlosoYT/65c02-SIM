#include "Frontend/HeadlessRunner.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

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
        std::cerr << "Headless: No ROM provided and fallback ROM not found.\n"
                  << "Tried: output/rom/room.bin, ./output/rom/room.bin, room.bin\n";
        return -1;
    }
    if (args.romPath.empty()) {
        std::cerr << "Headless: No ROM specified, using fallback ROM: " << romPath << '\n';
    }

    std::string errorMsg;
    if (!emulator.Init(romPath, errorMsg)) {
        std::cerr << "Headless: Failed to load ROM: " << errorMsg << '\n';
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
        std::cout << std::fixed << std::setprecision(3)
                  << "SID profile: clocks=" << stats.clockCount
                  << ", oscillator_ms=" << toMilliseconds(stats.oscillatorNanoseconds)
                  << ", envelope_ms=" << toMilliseconds(stats.envelopeNanoseconds)
                  << ", filter_ms=" << toMilliseconds(stats.filterNanoseconds)
                  << ", resampler_ms=" << toMilliseconds(stats.resamplerNanoseconds)
                  << ", output_ms=" << toMilliseconds(stats.outputNanoseconds) << '\n';
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
            std::cerr << "Headless: Failed to open dump file " << args.dumpMemPath << '\n';
        }
    }

    emulator.Stop();
    emulator.GetSID().Close();
    return 0;
}

}  // namespace Frontend
