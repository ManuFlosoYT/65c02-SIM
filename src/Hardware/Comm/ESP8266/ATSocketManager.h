#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <asio.hpp>
#ifndef TARGET_WASM
#include <asio/ssl.hpp>
#endif

#include "Hardware/Comm/ESP8266/ATConnection.h"

namespace Hardware {

class ESP8266;
class ATHttpClient;

static constexpr int kMaxConnections = 5;

class ATSocketManager {
public:
    explicit ATSocketManager(ESP8266& esp);
    ~ATSocketManager();

    ATSocketManager(const ATSocketManager&) = delete;
    ATSocketManager& operator=(const ATSocketManager&) = delete;
    ATSocketManager(ATSocketManager&&) = delete;
    ATSocketManager& operator=(ATSocketManager&&) = delete;

    void Reset();

    void SetHttpClient(ATHttpClient* client) { httpClient = client; }

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

    [[nodiscard]] int FindFreeLinkId() const;
    [[nodiscard]] bool IsAnyConnectionActive() const;
    [[nodiscard]] bool IsConnectionActive(int linkId) const;
    [[nodiscard]] const ATConnection& GetConnection(int linkId) const;

    [[nodiscard]] std::string GetLocalIP();

private:
    void RxLoopTCP(int linkId);
#ifndef TARGET_WASM
    void RxLoopSSL(int linkId);
#endif
    void RxLoopUDP(int linkId);
    void AcceptLoop();

    ESP8266& esp;
    ATHttpClient* httpClient{nullptr};

    asio::io_context ioContext;
#ifndef TARGET_WASM
    asio::ssl::context sslContext;
#endif
    std::array<ATConnection, kMaxConnections> connections;

    std::unique_ptr<asio::ip::tcp::acceptor> serverAcceptor;
    std::thread acceptThread;
    std::atomic<bool> serverRunning{false};
};

}  // namespace Hardware
