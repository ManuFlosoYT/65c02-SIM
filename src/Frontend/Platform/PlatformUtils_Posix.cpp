#include "Frontend/Platform/PlatformUtils.h"

#include <SDL3/SDL.h>
#include <execinfo.h>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    return SDL_OpenURL(nullTerminatedUrl.c_str());
}

std::filesystem::path GetExecutableDirectory() {
    std::error_code ec;
    auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !exePath.empty()) {
        return exePath.parent_path();
    }
    return std::filesystem::current_path();
}

char GetEnvPathDelimiter() {
    return ':';
}

std::string AppendExecutableExtension(std::string_view binaryName) {
    return std::string(binaryName);
}

std::string CaptureStackTrace() {
    std::array<void*, 64> callstack{};
    int frames = backtrace(callstack.data(), static_cast<int>(callstack.size()));
    char** strs = backtrace_symbols(callstack.data(), frames);
    if (!strs) {
        return "No stack trace available.";
    }
    std::string result;
    for (int i = 0; i < frames; ++i) {
        result += strs[i];
        result += '\n';
    }
    free(strs);
    return result;
}

}  // namespace Frontend::Platform
