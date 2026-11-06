#pragma once

#include "Frontend/Control/AppState.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace Control {

class IConfigPlatform {
public:
    virtual ~IConfigPlatform() = default;
    virtual void SetAutoReload(AppState& state, bool enabled) = 0;
    virtual bool CheckFileModified(const std::filesystem::path& path) = 0;
    virtual void UpdateFileTimestamp(const std::filesystem::path& path) = 0;
};

class NullConfigPlatform final : public IConfigPlatform {
public:
    void SetAutoReload(AppState&, bool) override {}
    bool CheckFileModified(const std::filesystem::path&) override { return false; }
    void UpdateFileTimestamp(const std::filesystem::path&) override {}
};

class DesktopConfigPlatform final : public IConfigPlatform {
public:
    void SetAutoReload(AppState& state, bool enabled) override;
    bool CheckFileModified(const std::filesystem::path& path) override;
    void UpdateFileTimestamp(const std::filesystem::path& path) override;

private:
    std::filesystem::file_time_type m_lastModifiedTime{};
    std::chrono::high_resolution_clock::time_point m_lastWatchCheck{};
};

class ConfigPlatform {
public:
    static ConfigPlatform& Instance();

    static void SetAutoReload(AppState& state, bool enabled) {
        Instance().m_impl->SetAutoReload(state, enabled);
    }

    static bool CheckFileModified(const std::filesystem::path& path) {
        return Instance().m_impl->CheckFileModified(path);
    }

    static void UpdateFileTimestamp(const std::filesystem::path& path) {
        Instance().m_impl->UpdateFileTimestamp(path);
    }

    void SetImplementation(std::shared_ptr<IConfigPlatform> impl) {
        m_impl = std::move(impl);
    }

private:
    ConfigPlatform();
    std::shared_ptr<IConfigPlatform> m_impl;
};

}  // namespace Control
