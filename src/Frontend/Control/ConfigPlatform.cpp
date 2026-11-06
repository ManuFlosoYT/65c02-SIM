#include "Frontend/Control/ConfigPlatform.h"

namespace Control {

void DesktopConfigPlatform::SetAutoReload(AppState& state, bool enabled) {
    state.emulator.SetAutoReload(enabled);
}

bool DesktopConfigPlatform::CheckFileModified(const std::filesystem::path& path) {
    using namespace std::chrono;
    auto now = high_resolution_clock::now();
    if (duration_cast<milliseconds>(now - m_lastWatchCheck).count() < 500) {
        return false;
    }
    m_lastWatchCheck = now;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return false;
    }

    auto latestTime = std::filesystem::last_write_time(path, ec);
    if (!ec && latestTime > m_lastModifiedTime) {
        m_lastModifiedTime = latestTime;
        return true;
    }
    return false;
}

void DesktopConfigPlatform::UpdateFileTimestamp(const std::filesystem::path& path) {
    std::error_code ec;
    auto lwt = std::filesystem::last_write_time(path, ec);
    if (!ec) {
        m_lastModifiedTime = lwt;
    }
}

ConfigPlatform::ConfigPlatform()
    : m_impl(std::make_shared<DesktopConfigPlatform>()) {}

ConfigPlatform& ConfigPlatform::Instance() {
    static ConfigPlatform instance;
    return instance;
}

}  // namespace Control
