#include "Hardware/Comm/ESP8266/ATPingPlatform.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <thread>

namespace Hardware::Comm::Platform {

std::optional<std::string> ExecutePing([[maybe_unused]] const std::string& host, const std::atomic<bool>& stopPing) {
    for (int i = 0; i < 50; ++i) {
        if (stopPing.load()) {
            return std::nullopt;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (stopPing.load()) {
        return std::nullopt;
    }
    return std::make_optional("71");
}

}  // namespace Hardware::Comm::Platform
