#include "Hardware/Core/Emulator/BinaryFileWatcher.h"
#include "Hardware/Core/Emulator/Emulator.h"

#include <iostream>

namespace Core {

void BinaryFileWatcher::CheckAutoReload(Emulator& emulator) {
    CheckAutoReload(emulator, lastWatchCheck);
}

void BinaryFileWatcher::CheckAutoReload(Emulator& emulator, std::chrono::high_resolution_clock::time_point& watchCheck) {
#ifndef TARGET_WASM
    using namespace std::chrono;
    const std::string binPath = emulator.GetCurrentBinPath();
    if (autoReloadRequested.load(std::memory_order_relaxed) && !binPath.empty()) {
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - watchCheck).count() >= 500) {
            watchCheck = now;
            try {
                if (std::filesystem::exists(binPath)) {
                    auto latestTime = std::filesystem::last_write_time(binPath);
                    if (latestTime > lastBinModificationTime) {
                        std::string errorMsg;
                        std::cerr << "Auto-reloading: " << binPath << '\n';
                        if (!emulator.Init(binPath, errorMsg)) {
                            std::cerr << "Auto-reload failed: " << errorMsg << '\n';
                        }
                    }
                }
            } catch (...) {
                (void)0;
            }
        }
    }
#else
    (void)emulator;
    (void)watchCheck;
#endif
}

}  // namespace Core
