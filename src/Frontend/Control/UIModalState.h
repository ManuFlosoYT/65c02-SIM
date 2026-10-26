#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace Control {

enum class DebuggerMode : std::uint8_t {
    Settings = 0,
    Disassembly = 1,
    Profiler = 2,
    Debugger = 3,
    Breakpoints = 4,
    MemoryLayout = 5,
    DataInspector = 6,
    LogicAnalyzer = 7,
    NetworkSniffer = 8,
    MemoryScanner = 9
};

struct NetworkSnifferState {
    bool autoScroll = true;
    bool paused = false;
    int filterDirection = 0;  // 0=All, 1=TX, 2=RX, 3=NetTX, 4=NetRX
    char searchFilter[64] = "";
    int selectedPacketIndex = -1;
    bool aciaBridgeEnabled = false;
};

struct DebuggerState {
    bool open = false;
    DebuggerMode mode = DebuggerMode::Settings;
    bool heatmapEnabled = true;
};

struct UpdateState {
    std::atomic<bool> available{false};
    std::string latestVersionTag;

    UpdateState() = default;
    UpdateState(const UpdateState& other)
        : available(other.available.load()),
          latestVersionTag(other.latestVersionTag) {}
    UpdateState& operator=(const UpdateState& other) {
        if (this != &other) {
            available.store(other.available.load());
            latestVersionTag = other.latestVersionTag;
        }
        return *this;
    }
};

enum class PendingCartridgeAction : std::uint8_t {
    None,
    Reset,
    Eject,
    Exit
};

struct PopupState {
    bool sdCardDisabled = false;
    bool sdCardWebWarning = false;
    bool sdCardMountSentinelWarning = false;
    std::string sdCardPendingMountFolder;
    bool showCartridgeSavePrompt = false;
    PendingCartridgeAction pendingCartridgeAction = PendingCartridgeAction::None;
};

struct SDKState {
    std::vector<std::string> roms;
    std::vector<std::string> midis;
    std::vector<std::string> vrams;
    bool loaded = false;
    bool showPopup = false;
    bool showUpdatePopup = false;
    std::string extractedVersion;
};

}  // namespace Control
