#include "Frontend/CliArgsParser.h"

#include <charconv>
#include <iostream>
#include <string_view>

namespace Frontend {

namespace {

bool ParseStrictInt(std::string_view text, int& outValue) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(begin, end, outValue);
    return ec == std::errc() && ptr == end;
}

}  // namespace

void CliArgsParser::PrintUsage(const char* exeName) {
    std::cerr << "Usage: " << exeName << " [OPTIONS] [ROM_PATH]\n"
              << "Options:\n"
              << "  --headless              Run without GUI\n"
              << "  --rom <path>            Load ROM from path (fallback: output/rom/room.bin)\n"
              << "  --script <path>         Load and run Python script\n"
              << "  --run-cycles <N>        Run N cycles (N >= 0)\n"
              << "  --sid-sound <on|off>    Enable or disable SID audio processing\n"
              << "  --sid-device <on|off>  Enable or disable the SID device\n"
              << "  --profile-sid           Print SID timing counters\n"
              << "  --dump-mem <file>       Dump 64KB memory to file\n"
              << "  --help                  Show this help\n";
}

ParseArgsResult CliArgsParser::Parse(std::span<const char* const> argv) {
    ParseArgsResult result;
    Args& args = result.args;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        std::string_view arg = argv[i];
        if (arg == "--help") {
            result.showUsage = true;
            return result;
        }
        if (arg == "--headless") {
            args.headless = true;
        } else if (arg == "--profile-sid") {
            args.profileSID = true;
        } else if (arg == "--sid-sound") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --sid-sound";
                return result;
            }
            std::string_view value = argv[++i];
            if (value == "on") {
                args.sidSound = true;
            } else if (value == "off") {
                args.sidSound = false;
            } else {
                result.ok = false;
                result.error = "Invalid value for --sid-sound: " + std::string(value);
                return result;
            }
        } else if (arg == "--sid-device") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --sid-device";
                return result;
            }
            std::string_view value = argv[++i];
            if (value == "on") {
                args.sidDevice = true;
            } else if (value == "off") {
                args.sidDevice = false;
            } else {
                result.ok = false;
                result.error = "Invalid value for --sid-device: " + std::string(value);
                return result;
            }
        } else if (arg == "--script") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --script";
                return result;
            }
            args.scriptPath = argv[++i];
        } else if (arg == "--run-cycles") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --run-cycles";
                return result;
            }
            int parsedCycles = 0;
            if (!ParseStrictInt(argv[++i], parsedCycles) || parsedCycles < 0) {
                result.ok = false;
                result.error = "Invalid value for --run-cycles: " + std::string(argv[i]);
                return result;
            }
            args.runCycles = parsedCycles;
        } else if (arg == "--dump-mem") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --dump-mem";
                return result;
            }
            args.dumpMemPath = argv[++i];
        } else if (arg == "--rom") {
            if (i + 1 >= argv.size()) {
                result.ok = false;
                result.error = "Missing value for --rom";
                return result;
            }
            args.romPath = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            result.ok = false;
            result.error = "Unknown option: " + std::string(arg);
            return result;
        } else if (args.romPath.empty()) {
            args.romPath = std::string(arg);
        } else {
            result.ok = false;
            result.error = "Unexpected positional argument: " + std::string(arg);
            return result;
        }
    }
    return result;
}

}  // namespace Frontend
