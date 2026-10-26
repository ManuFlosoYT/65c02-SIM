#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

namespace Core {

class Emulator;

class BinaryFileWatcher {
public:
    BinaryFileWatcher() = default;

    void SetEnabled(bool enabled) { autoReloadRequested.store(enabled, std::memory_order_relaxed); }
    [[nodiscard]] bool IsEnabled() const { return autoReloadRequested.load(std::memory_order_relaxed); }

    void SetLastBinModificationTime(std::filesystem::file_time_type t) { lastBinModificationTime = t; }
    [[nodiscard]] std::filesystem::file_time_type GetLastBinModificationTime() const { return lastBinModificationTime; }

    void CheckAutoReload(Emulator& emulator);
    void CheckAutoReload(Emulator& emulator, std::chrono::high_resolution_clock::time_point& lastWatchCheck);

private:
    std::filesystem::file_time_type lastBinModificationTime{};
    std::atomic<bool> autoReloadRequested{true};
    std::chrono::high_resolution_clock::time_point lastWatchCheck{std::chrono::high_resolution_clock::now()};
};

}  // namespace Core
