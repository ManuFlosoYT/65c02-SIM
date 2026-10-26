#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "Hardware/Core/IBusDevice.h"
#include "Hardware/Comm/ESP8266/ATHandler.h"
#include "Hardware/Comm/ESP8266/NetworkLoop.h"
#include "Hardware/Comm/ESP8266/NetworkTrafficLogger.h"

namespace Hardware {

class ESP8266 : public IBusDevice {
public:
    ESP8266();
    ~ESP8266() override;

    ESP8266(const ESP8266&) = delete;
    ESP8266& operator=(const ESP8266&) = delete;
    ESP8266(ESP8266&&) = delete;
    ESP8266& operator=(ESP8266&&) = delete;

    void Reset() override;

    Byte Read(Word address) override;
    Byte ReadDirect(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

    inline void RecordTraffic(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                              const std::string& payload) {
        trafficLogger.Record(dir, ch, linkId, tag, payload);
    }
    [[nodiscard]] inline std::vector<TrafficPacket> GetTrafficLog() const { return trafficLogger.GetTrafficLog(); }
    inline void ClearTrafficLog() { trafficLogger.ClearTrafficLog(); }
    inline void SetSnifferPaused(bool paused) { trafficLogger.SetSnifferPaused(paused); }
    [[nodiscard]] inline bool IsSnifferPaused() const { return trafficLogger.IsSnifferPaused(); }
    [[nodiscard]] NetworkTrafficLogger& GetTrafficLogger() { return trafficLogger; }
    [[nodiscard]] const NetworkTrafficLogger& GetTrafficLogger() const { return trafficLogger; }
    void SetRxByteCallback(std::function<void(Byte)> callback);

    void EnqueueResponse(const std::string& response);
    void EnqueueIPD(int linkId, const char* data, size_t bytesRead);
    void EnqueueLinkClosedResponse(int linkId);
    [[nodiscard]] bool IsRxBufferFull() const;
    [[nodiscard]] bool IsMuxEnabled() const;

    [[nodiscard]] ATHandler& GetATHandler() { return atHandler; }
    [[nodiscard]] const ATHandler& GetATHandler() const { return atHandler; }
    [[nodiscard]] NetworkLoop& GetNetworkLoop() { return networkLoop; }
    [[nodiscard]] const NetworkLoop& GetNetworkLoop() const { return networkLoop; }

private:
    std::atomic<Byte> statusReg{0};
    Byte ctrlReg{0};

    std::queue<Byte> rxQueue;
    mutable std::mutex rxMutex;

    NetworkTrafficLogger trafficLogger;
    std::function<void(Byte)> rxByteCallback;

    NetworkLoop networkLoop;
    ATHandler atHandler;
};

}  // namespace Hardware
