#include "Frontend/Platform/PlatformUtils.h"

#include <SDL3/SDL.h>
#include <windows.h>
#include <array>
#include <filesystem>
#include <string>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    return SDL_OpenURL(nullTerminatedUrl.c_str());
}

std::filesystem::path GetExecutableDirectory() {
    std::array<char, MAX_PATH> buffer{};
    if (GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) > 0) {
        return std::filesystem::path(buffer.data()).parent_path();
    }
    return std::filesystem::current_path();
}

char GetEnvPathDelimiter() {
    return ';';
}

std::string AppendExecutableExtension(std::string_view binaryName) {
    if (!binaryName.ends_with(".exe")) {
        return std::string(binaryName) + ".exe";
    }
    return std::string(binaryName);
}

}  // namespace Frontend::Platform
