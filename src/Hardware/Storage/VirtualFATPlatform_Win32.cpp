#include "Hardware/Storage/VirtualFATPlatform.h"

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <vector>

namespace Hardware::Storage::Platform {

bool ComponentEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool SafeLocalTime(const std::time_t* time, std::tm* result) {
    if (!time || !result) {
        return false;
    }
    return localtime_s(result, time) == 0;
}

std::vector<std::filesystem::path> GetProtectedDirectories() {
    std::vector<std::filesystem::path> protectedDirs;
    for (const char* env : {"SystemRoot", "WINDIR", "ProgramFiles", "ProgramFiles(x86)", "ProgramData", "USERPROFILE"}) {
        if (const char* val = std::getenv(env)) {
            protectedDirs.emplace_back(val);
        }
    }
    return protectedDirs;
}

}  // namespace Hardware::Storage::Platform
