#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace Hardware {
class ACIA;
class ESP8266;
}

namespace Core {

class SerialIOBridge {
public:
    SerialIOBridge() = default;
    ~SerialIOBridge() = default;

    SerialIOBridge(const SerialIOBridge&) = delete;
    SerialIOBridge& operator=(const SerialIOBridge&) = delete;
    SerialIOBridge(SerialIOBridge&&) = delete;
    SerialIOBridge& operator=(SerialIOBridge&&) = delete;

    void Clear();
    void InjectKey(char key);

    [[nodiscard]] inline bool HasInput() const noexcept {
        return hasInput.load(std::memory_order_relaxed);
    }

    [[nodiscard]] inline int GetBaudDelay() const noexcept { return baudDelay; }
    inline void SetBaudDelay(int delay) noexcept { baudDelay = delay; }

    inline void DecrementBaudDelay() noexcept {
        if (baudDelay > 0) [[unlikely]] {
            baudDelay--;
        }
    }

    void ProcessSerialInput(Hardware::ACIA& acia, int targetIps, std::atomic<bool>& pendingInterruptAny);
    void ForwardOutputIfBridged(char c, Hardware::ESP8266& esp8266);

    void SerializeBuffer(std::ostream& out) const;
    bool DeserializeBuffer(std::istream& inStream);

    void SetACIAESPBridge(bool enabled) noexcept { aciaEspBridge = enabled; }
    [[nodiscard]] bool IsACIAESPBridge() const noexcept { return aciaEspBridge; }

private:
    std::deque<char> inputBuffer;
    mutable std::mutex bufferMutex;
    std::atomic<bool> hasInput{false};
    int baudDelay = 0;
    bool aciaEspBridge = false;
};

} // namespace Core

