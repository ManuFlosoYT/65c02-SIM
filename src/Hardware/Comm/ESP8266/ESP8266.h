#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <asio.hpp>
#ifndef TARGET_WASM
#include <asio/ssl.hpp>
#endif

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

static constexpr int kMaxConnections = 5;

enum class ATState : uint8_t {
    Idle,
    WaitingCIPSENDData
};

enum class CWMode : uint8_t {
    Station = 1,
    SoftAP = 2,
    StationAndAP = 3
};

enum class TrafficDirection : uint8_t {
    TX,
    RX,
    NetTX,
    NetRX
};

enum class TrafficChannel : uint8_t {
    ATCommand,
    ATResponse,
    DataPayload,
    TLS_HTTP,
    System
};

struct TrafficPacket {
    uint64_t timestampMs{0};
    TrafficDirection direction{TrafficDirection::TX};
    TrafficChannel channel{TrafficChannel::ATCommand};
    int linkId{-1};
    std::string tag;
    std::string payload;
};

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

    void RecordTraffic(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                       const std::string& payload);
    [[nodiscard]] std::vector<TrafficPacket> GetTrafficLog() const;
    void ClearTrafficLog();
    void SetSnifferPaused(bool paused);
    [[nodiscard]] bool IsSnifferPaused() const;
    void SetRxByteCallback(std::function<void(Byte)> callback);

   private:
    void ProcessCommand();
    void EnqueueResponse(const std::string& response);
    void EnqueueLinkClosedResponse(int linkId);

    void DispatchATCommand(const std::string& upper, const std::string& original);

    // HTTP / HTTPS commands
    void HandleHTTPCLIENT(const std::string& cmd);
#ifndef TARGET_WASM
    void PerformHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData);
    void PerformHttpClientRequest(int opt, int contentType, const std::string& url, const std::string& host,
                                  const std::string& path, const std::string& postData);
#endif

    // WiFi commands
    void HandleCWMode(const std::string& cmd);
    void HandleCWJAP(const std::string& cmd);
    void HandleCWLAP();
    void HandleCWQAP();
    void HandleCIFSR();

    // Connection commands
    void HandleCIPSTART(const std::string& cmd);
    void HandleCIPSEND(const std::string& cmd);
    void HandleCIPCLOSE(const std::string& cmd);
    void HandleCIPMUX(const std::string& cmd);
    void HandleCIPSTATUS();
    void HandleCIPSERVER(const std::string& cmd);

    // System commands
    void HandleGMR();
    void HandlePing(const std::string& cmd);

    // Connection management
    void ConnectTCP(int linkId, const std::string& host, int port);
    void ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode);
#ifndef TARGET_WASM
    void ConnectSSL(int linkId, const std::string& host, int port);
#endif
    void DisconnectLink(int linkId);
    void DisconnectAll();
    void RxLoopTCP(int linkId);
#ifndef TARGET_WASM
    void RxLoopSSL(int linkId);
#endif
    void RxLoopUDP(int linkId);
    void SendDataOnLink(int linkId, const std::string& data);

    // Server
    void StartServer(int port);
    void StopServer();
    void AcceptLoop();

    // Ping
    void PingTask(const std::string& host);
    std::thread pingThread;
    std::atomic<bool> stopPing{false};
    std::thread httpClientThread;

    // Helpers
    [[nodiscard]] int FindFreeLinkId() const;
    [[nodiscard]] bool IsAnyConnectionActive() const;
    static std::string ParseQuotedParam(const std::string& cmd, size_t& pos);
    static int ParseIntParam(const std::string& cmd, size_t& pos);
    void EnqueueIPD(int linkId, const char* data, size_t bytesRead);

    // CIPSTART helpers
    void PerformConnection(int linkId, const std::string& type, const std::string& host, int port, const std::string& params, size_t pos);

    // Registers (memory-mapped)
    std::atomic<Byte> statusReg;
    Byte ctrlReg;

    // AT state machine
    ATState currentState{ATState::Idle};
    std::string commandBuffer;
    std::queue<Byte> rxQueue;
    std::mutex rxMutex;

    // CIPSEND state
    int cipsendLinkId{0};
    int cipsendLength{0};
    int cipsendReceived{0};
    std::string cipsendBuffer;

    // Config
    bool echoEnabled{true};
    std::atomic<bool> muxEnabled{false};
    bool wifiConnected{false};
    CWMode cwMode{CWMode::Station};
    std::string connectedSSID;

    // Networking
    asio::io_context ioContext;
#ifndef TARGET_WASM
    asio::ssl::context sslContext;
#endif
    std::array<ATConnection, kMaxConnections> connections;

    // Server
    std::unique_ptr<asio::ip::tcp::acceptor> serverAcceptor;
    std::thread acceptThread;
    std::atomic<bool> serverRunning{false};

    // Sniffer
    static constexpr size_t kMaxTrafficPackets = 500;
    std::deque<TrafficPacket> trafficLog;
    mutable std::mutex trafficMutex;
    std::atomic<bool> snifferPaused{false};
    std::function<void(Byte)> rxByteCallback;
};

}  // namespace Hardware
