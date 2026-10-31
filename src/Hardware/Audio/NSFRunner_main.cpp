#include <iomanip>
#include <iostream>
#include <span>

#include "Frontend/CliArgsParser.h"
#include "Hardware/Audio/NSFRunner.h"

int main(int argc, char* argv[]) {
    const auto parsed =
        Frontend::CliArgsParser::Parse(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));
    if (!parsed.ok) {
        std::cerr << parsed.error << '\n';
        Frontend::CliArgsParser::PrintUsage(argv[0]);
        return 1;
    }
    if (parsed.showUsage) {
        Frontend::CliArgsParser::PrintUsage(argv[0]);
        return 0;
    }

    const auto& args = parsed.args;
    if (args.nsfTracePath.empty() || args.romPath.empty()) {
        std::cerr << "[NSFRunner] Minimal core requires --rom <file.nsf> --nsf-trace <out.json>\n";
        return 1;
    }

    const Hardware::NSFRunConfig config{
        .maxDurationSec = args.nsfMaxDuration,
        .songIndex = args.nsfSong,
    };
    const Hardware::NSFRunResult res = Hardware::NSFRunner::RunFile(args.romPath, config);
    if (!res.success) {
        std::cerr << "[NSFRunner] Error: " << res.errorMessage << '\n';
        return 1;
    }
    if (!Hardware::NSFRunner::WriteTraceJson(res, args.nsfTracePath)) {
        std::cerr << "[NSFRunner] Error: Failed to write trace JSON to " << args.nsfTracePath << '\n';
        return 1;
    }
    std::cout << std::fixed << std::setprecision(2) << "[NSFRunner] Trace written to " << args.nsfTracePath << " ("
              << res.totalFrames << " frames, " << res.durationSec << "s, stop=" << res.stopReason << ")\n";
    return 0;
}

