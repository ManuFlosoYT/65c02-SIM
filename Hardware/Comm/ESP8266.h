#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <asio.hpp>
#ifndef TARGET_WASM
#include <asio/ssl.hpp>
#endif

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

static constexpr int kMaxConnections = 5;
static constexpr int kDefaultServerTimeout = 180;

enum class ATState : uint8_t {
    Idle,
    WaitingCIPSENDData,
    PassthroughMode
};

enum class CWMode : uint8_t {
    Station = 1,
    SoftAP = 2,
    StationAndAP = 3
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
    std::atomic<bool> active{false};
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
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

    [[nodiscard]] bool HasIRQ() const { return (statusReg.load(std::memory_order_relaxed) & 0x80) != 0; }

    void Clock();

   private:
    void ProcessCommand();
    void EnqueueResponse(const std::string& response);
    void EnqueueLinkClosedResponse(int linkId);

    void DispatchATCommand(const std::string& upper, const std::string& original);

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

    // Helpers
    [[nodiscard]] int FindFreeLinkId() const;
    [[nodiscard]] bool IsAnyConnectionActive() const;
    static std::string ParseQuotedParam(const std::string& cmd, size_t& pos);
    static int ParseIntParam(const std::string& cmd, size_t& pos);

    // CIPSTART helpers
    void ParseCIPSTARTParams(const std::string& params, int& linkId, std::string& type, std::string& host, int& port, size_t& pos) const;
    void PerformConnection(int linkId, const std::string& type, const std::string& host, int port, const std::string& params, size_t pos);

    // Registers (memory-mapped)
    std::atomic<Byte> statusReg;
    Byte cmdReg;
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
    bool muxEnabled{false};
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
    int serverTimeout{kDefaultServerTimeout};
};

}  // namespace Hardware
