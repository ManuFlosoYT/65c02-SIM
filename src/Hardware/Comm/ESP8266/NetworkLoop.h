#pragma once

#include <string>

#include "Hardware/Comm/ESP8266/ATSocketManager.h"
#include "Hardware/Comm/ESP8266/ATHttpClient.h"
#include "Hardware/Comm/ESP8266/ATPingService.h"

namespace Hardware {

class ESP8266;

class NetworkLoop {
public:
    explicit NetworkLoop(ESP8266& esp);
    ~NetworkLoop();

    NetworkLoop(const NetworkLoop&) = delete;
    NetworkLoop& operator=(const NetworkLoop&) = delete;
    NetworkLoop(NetworkLoop&&) = delete;
    NetworkLoop& operator=(NetworkLoop&&) = delete;

    void Reset();

    void ConnectTCP(int linkId, const std::string& host, int port);
    void ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode);
#ifndef TARGET_WASM
    void ConnectSSL(int linkId, const std::string& host, int port);
#endif
    void DisconnectLink(int linkId);
    void DisconnectAll();
    void SendDataOnLink(int linkId, const std::string& data);

    void StartServer(int port);
    void StopServer();

    void StartPing(const std::string& host);

#ifndef TARGET_WASM
    void StartHttpClientRequest(int opt, int contentType, const std::string& url,
                                const std::string& host, const std::string& path,
                                const std::string& postData);
#endif

    [[nodiscard]] int FindFreeLinkId() const;
    [[nodiscard]] bool IsAnyConnectionActive() const;
    [[nodiscard]] bool IsConnectionActive(int linkId) const;
    [[nodiscard]] const ATConnection& GetConnection(int linkId) const;

    [[nodiscard]] std::string GetLocalIP();

    [[nodiscard]] ATSocketManager& GetSocketManager() { return socketManager; }
    [[nodiscard]] const ATSocketManager& GetSocketManager() const { return socketManager; }
    [[nodiscard]] ATHttpClient& GetHttpClient() { return httpClient; }
    [[nodiscard]] const ATHttpClient& GetHttpClient() const { return httpClient; }
    [[nodiscard]] ATPingService& GetPingService() { return pingService; }
    [[nodiscard]] const ATPingService& GetPingService() const { return pingService; }

private:
    ESP8266& esp;
    ATSocketManager socketManager;
    ATHttpClient httpClient;
    ATPingService pingService;
};

}  // namespace Hardware
