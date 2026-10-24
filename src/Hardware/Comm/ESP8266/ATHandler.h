#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class ESP8266;
class NetworkLoop;

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

class ATHandler {
public:
    explicit ATHandler(ESP8266* esp = nullptr);
    ~ATHandler() = default;

    ATHandler(const ATHandler&) = delete;
    ATHandler& operator=(const ATHandler&) = delete;
    ATHandler(ATHandler&&) = delete;
    ATHandler& operator=(ATHandler&&) = delete;

    void Reset();

    void HandleWrite(Byte data);
    void ProcessCommand(const std::string& cmd);

    void SetNetworkLoop(NetworkLoop* loop);
    [[nodiscard]] NetworkLoop* GetNetworkLoop() const;

    [[nodiscard]] bool IsMuxEnabled() const;
    void SetMuxEnabled(bool enabled);

    [[nodiscard]] CWMode GetCWMode() const;
    void SetCWMode(CWMode mode);

    [[nodiscard]] bool IsWifiConnected() const;
    void SetWifiConnected(bool connected);

    [[nodiscard]] const std::string& GetConnectedSSID() const;
    void SetConnectedSSID(const std::string& ssid);

    [[nodiscard]] bool IsEchoEnabled() const;
    void SetEchoEnabled(bool enabled);

    [[nodiscard]] ATState GetState() const;
    void SetState(ATState state);

    [[nodiscard]] const std::string& GetCommandBuffer() const;
    void SetCommandBuffer(const std::string& buf);

    void EnqueueResponse(const std::string& response);
    void RecordTraffic(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                       const std::string& payload);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);

    void SetResponseCallback(std::function<void(const std::string&)> cb);
    [[nodiscard]] const std::string& GetLastResponse() const;

    static std::string ParseQuotedParam(const std::string& cmd, size_t& pos);
    static int ParseIntParam(const std::string& cmd, size_t& pos);

private:
    void ProcessCurrentCommandBuffer();
    void DispatchATCommand(const std::string& upper, const std::string& original);

    void HandleGMR();
    void HandleCWMode(const std::string& cmd);
    void HandleCWJAP(const std::string& cmd);
    void HandleCWLAP();
    void HandleCWQAP();
    void HandleCIFSR();
    void HandleCIPSTART(const std::string& cmd);
    void HandleCIPSEND(const std::string& cmd);
    void HandleCIPCLOSE(const std::string& cmd);
    void HandleCIPMUX(const std::string& cmd);
    void HandleCIPSTATUS();
    void HandleCIPSERVER(const std::string& cmd);
    void HandleHTTPCLIENT(const std::string& cmd);
    void HandlePing(const std::string& cmd);

    void PerformConnection(int linkId, const std::string& type, const std::string& host, int port,
                           const std::string& params, size_t pos);

    ESP8266* esp{nullptr};
    NetworkLoop* networkLoop{nullptr};

    ATState currentState{ATState::Idle};
    std::string commandBuffer;

    int cipsendLinkId{0};
    int cipsendLength{0};
    int cipsendReceived{0};
    std::string cipsendBuffer;

    bool echoEnabled{true};
    std::atomic<bool> muxEnabled{false};
    bool wifiConnected{false};
    CWMode cwMode{CWMode::Station};
    std::string connectedSSID;

    std::function<void(const std::string&)> responseCallback;
    std::string lastResponse;
};

}  // namespace Hardware
