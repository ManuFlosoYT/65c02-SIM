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

namespace Hardware {

class ESP8266;

static constexpr int kMaxConnections = 5;

struct ATConnection {
    std::unique_ptr<asio::ip::tcp::socket> tcpSocket;
#ifndef TARGET_WASM
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslStream;
#endif
    std::unique_ptr<asio::ip::udp::socket> udpSocket;
    asio::ip::udp::endpoint udpRemoteEndpoint;
    int udpMode{0};
    std::thread rxThread;
    std::recursive_mutex rxThreadMutex;
    std::atomic<bool> active{false};
    bool isHttps{false};
    std::string protocol;
    std::string remoteHost;
    int remotePort{0};

    void StopRxThread();
};

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

private:
    void RxLoopTCP(int linkId);
#ifndef TARGET_WASM
    void RxLoopSSL(int linkId);
    void PerformHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData);
    void PerformHttpClientRequest(int opt, int contentType, const std::string& url,
                                  const std::string& host, const std::string& path,
                                  const std::string& postData);
#endif
    void RxLoopUDP(int linkId);
    void AcceptLoop();
    void PingTask(const std::string& host);

    ESP8266& esp;

    asio::io_context ioContext;
#ifndef TARGET_WASM
    asio::ssl::context sslContext;
#endif
    std::array<ATConnection, kMaxConnections> connections;

    std::unique_ptr<asio::ip::tcp::acceptor> serverAcceptor;
    std::thread acceptThread;
    std::atomic<bool> serverRunning{false};

    std::thread pingThread;
    std::atomic<bool> stopPing{false};
    std::thread httpClientThread;
};

}  // namespace Hardware
