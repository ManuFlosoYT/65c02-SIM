#include "Hardware/Storage/VirtualFATPlatform.h"

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <vector>

namespace Hardware::Storage::Platform {

bool ComponentEquals(std::string_view a, std::string_view b) {
    return a == b;
}

bool SafeLocalTime(const std::time_t* time, std::tm* result) {
    if (!time || !result) {
        return false;
    }
    return localtime_r(time, result) != nullptr;
}

std::vector<std::filesystem::path> GetProtectedDirectories() {
    std::vector<std::filesystem::path> protectedDirs = {
        "/home", "/root", "/usr", "/bin", "/sbin", "/lib", "/etc", "/var", "/tmp",
        "/sys", "/proc", "/dev", "/boot", "/opt", "/Users", "/System", "/Library",
        "/Applications"
    };
    if (const char* home = std::getenv("HOME")) {
        protectedDirs.emplace_back(home);
    }
    return protectedDirs;
}

}  // namespace Hardware::Storage::Platform
