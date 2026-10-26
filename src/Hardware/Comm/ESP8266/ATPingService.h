#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace Hardware {

class ESP8266;

class ATPingService {
public:
    explicit ATPingService(ESP8266& esp);
    ~ATPingService();

    ATPingService(const ATPingService&) = delete;
    ATPingService& operator=(const ATPingService&) = delete;
    ATPingService(ATPingService&&) = delete;
    ATPingService& operator=(ATPingService&&) = delete;

    void Reset();
    void StartPing(const std::string& host);
    void StopPing();

private:
    void PingTask(const std::string& host);

    ESP8266& esp;
    std::thread pingThread;
    std::atomic<bool> stopPing{false};
};

}  // namespace Hardware
