#pragma once

#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

#include "Hardware/Core/IBusDevice.h"

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

    inline Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] inline std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

    [[nodiscard]] bool HasIRQ() const { return (statusReg & 0x80) != 0; }

    void Clock();

   private:
    void ProcessCommand();
    void EnqueueResponse(const std::string& response);
    
    void HandleATCommand(const std::string& cmd);
    
    void ConnectTCP(const std::string& host, int port);
    void DisconnectTCP();

    void HandleHTTPGet(const std::string& originalCmd);
    void HttpGetTask(std::string url);

    Byte statusReg;
    Byte cmdReg;
    Byte ctrlReg;

    std::string commandBuffer;
    std::queue<Byte> rxQueue;
    std::mutex rxMutex;
    
    std::atomic<bool> connected;
#ifdef _WIN32
    uint64_t sockfd;
#else
    int sockfd;
#endif

    std::thread rxThread;
    std::atomic<bool> threadRunning;
    void RxLoop();
};

}  // namespace Hardware



namespace Hardware {

inline std::string ESP8266::GetName() const { return "ESP8266"; }

inline Byte ESP8266::Read(Word address) {
    switch (address & 0x03) {
        case 0: {
            std::lock_guard<std::mutex> lock(rxMutex);
            if (rxQueue.empty()) {
                statusReg &= ~0x80;
                return 0;
            }
            Byte byte_val = rxQueue.front();
            rxQueue.pop();
            if (rxQueue.empty()) {
                statusReg &= ~0x80;
            }
            return byte_val;
        }
        case 1:
            return statusReg;
        case 2:
            return cmdReg;
        case 3:
            return ctrlReg;
        default:
            return 0;
    }
}

}  // namespace Hardware
