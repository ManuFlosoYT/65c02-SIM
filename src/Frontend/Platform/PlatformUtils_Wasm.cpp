#include "Frontend/Platform/PlatformUtils.h"

#include <emscripten.h>
#include <filesystem>
#include <format>
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

std::string CaptureStackTrace() {
    return "[WASM execution stack: check browser developer console]";
}

void InstallCrashHandler() {}
void EnsurePlatformDependencies() {}

bool RunProtected(const std::function<void()>& fn, std::string& outErrorMessage) {
    try {
        fn();
        return true;
    } catch (const std::exception& e) {
        outErrorMessage = std::format("Unhandled C++ exception: {}\n\nStack trace:\n{}", e.what(), CaptureStackTrace());
        return false;
    } catch (...) {
        outErrorMessage = "Unhandled unknown C++ exception";
        return false;
    }
}

}  // namespace Frontend::Platform
