#include "Frontend/Platform/PlatformUtils.h"

#include <emscripten.h>
#include <filesystem>
#include <string>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    std::string script = "window.open('" + nullTerminatedUrl + "', '_blank');";
    emscripten_run_script(script.c_str());
    return true;
}

std::filesystem::path GetExecutableDirectory() {
    return std::filesystem::current_path();
}

char GetEnvPathDelimiter() {
    return ':';
}

std::string AppendExecutableExtension(std::string_view binaryName) {
    return std::string(binaryName);
}

}  // namespace Frontend::Platform
