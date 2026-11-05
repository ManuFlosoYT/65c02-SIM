#pragma once

#include <atomic>
#include <optional>
#include <string>

namespace Hardware::Comm::Platform {

std::optional<std::string> ExecutePing(const std::string& host, const std::atomic<bool>& stopPing);

}  // namespace Hardware::Comm::Platform
