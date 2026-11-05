#include "Hardware/Comm/ESP8266/ATPingPlatform.h"

#include <atomic>
#include <cstdio>
#include <optional>
#include <string>

namespace Hardware::Comm::Platform {

std::optional<std::string> ExecutePing(const std::string& host, const std::atomic<bool>& stopPing) {
    std::string command = "ping -c 1 -W 2 " + host;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }

    char buffer[128];
    std::string result;
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        if (stopPing.load()) {
            pclose(pipe);
            return std::nullopt;
        }
    }
    pclose(pipe);

    size_t timePos = result.find("time=");
    if (timePos == std::string::npos) {
        timePos = result.find("time<");
    }

    if (timePos != std::string::npos) {
        size_t start = timePos + 5;
        size_t end = result.find("ms", start);
        if (end != std::string::npos) {
            std::string timeStr = result.substr(start, end - start);
            timeStr.erase(0, timeStr.find_first_not_of(" \t"));
            timeStr.erase(timeStr.find_last_not_of(" \t") + 1);
            return timeStr;
        }
    }
    return std::nullopt;
}

}  // namespace Hardware::Comm::Platform
