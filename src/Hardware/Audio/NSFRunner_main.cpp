#include <iomanip>
#include <iostream>
#include <span>

#include "Frontend/CliArgsParser.h"
#include "Hardware/Audio/NSFRunner.h"
#include "Hardware/Core/Logger.h"

int main(int argc, char* argv[]) {
    Core::Logger::Instance().SetMode(Core::LogMode::Console);
    const auto parsed =
        Frontend::CliArgsParser::Parse(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));
    if (!parsed.ok) {
        Core::Logger::Error("{}", parsed.error);
        Frontend::CliArgsParser::PrintUsage(argv[0]);
        return 1;
    }
    if (parsed.showUsage) {
        Frontend::CliArgsParser::PrintUsage(argv[0]);
        return 0;
    }

    const auto& args = parsed.args;
    if (args.nsfTracePath.empty() || args.romPath.empty()) {
        Core::Logger::Error("[NSFRunner] Minimal core requires --rom <file.nsf> --nsf-trace <out.json>");
        return 1;
    }

    const Hardware::NSFRunConfig config{
        .maxDurationSec = args.nsfMaxDuration,
        .songIndex = args.nsfSong,
    };
    const Hardware::NSFRunResult res = Hardware::NSFRunner::RunFile(args.romPath, config);
    if (!res.success) {
        Core::Logger::Error("[NSFRunner] Error: {}", res.errorMessage);
        return 1;
    }
    if (!Hardware::NSFRunner::WriteTraceJson(res, args.nsfTracePath)) {
        Core::Logger::Error("[NSFRunner] Error: Failed to write trace JSON to {}", args.nsfTracePath);
        return 1;
    }
    Core::Logger::Info("[NSFRunner] Trace written to {} ({} frames, {:.2f}s, stop={})", args.nsfTracePath,
                       res.totalFrames, res.durationSec, res.stopReason);
    return 0;
}

