#include "Frontend/Platform/PlatformUtils.h"

#include <SDL3/SDL.h>
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

}  // namespace Frontend::Platform
