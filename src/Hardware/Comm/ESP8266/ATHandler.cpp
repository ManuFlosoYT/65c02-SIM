#include "Hardware/Comm/ESP8266/ATHandler.h"
#include "Hardware/Comm/ESP8266/NetworkLoop.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <algorithm>
#include <cctype>

namespace Hardware {

ATHandler::ATHandler(ESP8266* espPtr)
    : esp(espPtr) {
    Reset();
}

void ATHandler::Reset() {
    currentState = ATState::Idle;
    commandBuffer.clear();
    cipsendBuffer.clear();
    cipsendLinkId = 0;
    cipsendLength = 0;
    cipsendReceived = 0;
    echoEnabled = true;
    muxEnabled.store(false);
    wifiConnected = false;
    connectedSSID.clear();
    cwMode = CWMode::Station;
}

void ATHandler::SetNetworkLoop(NetworkLoop* loop) { networkLoop = loop; }

NetworkLoop* ATHandler::GetNetworkLoop() const { return networkLoop; }

bool ATHandler::IsMuxEnabled() const { return muxEnabled.load(std::memory_order_relaxed); }

void ATHandler::SetMuxEnabled(bool enabled) { muxEnabled.store(enabled, std::memory_order_relaxed); }

CWMode ATHandler::GetCWMode() const { return cwMode; }

void ATHandler::SetCWMode(CWMode mode) { cwMode = mode; }

bool ATHandler::IsWifiConnected() const { return wifiConnected; }

void ATHandler::SetWifiConnected(bool connected) { wifiConnected = connected; }

const std::string& ATHandler::GetConnectedSSID() const { return connectedSSID; }

void ATHandler::SetConnectedSSID(const std::string& ssid) { connectedSSID = ssid; }

bool ATHandler::IsEchoEnabled() const { return echoEnabled; }

void ATHandler::SetEchoEnabled(bool enabled) { echoEnabled = enabled; }

ATState ATHandler::GetState() const { return currentState; }

void ATHandler::SetState(ATState state) { currentState = state; }

const std::string& ATHandler::GetCommandBuffer() const { return commandBuffer; }

void ATHandler::SetCommandBuffer(const std::string& buf) { commandBuffer = buf; }

void ATHandler::SetResponseCallback(std::function<void(const std::string&)> cb) {
    responseCallback = std::move(cb);
}

const std::string& ATHandler::GetLastResponse() const { return lastResponse; }

void ATHandler::EnqueueResponse(const std::string& response) {
    if (esp) {
        esp->EnqueueResponse(response);
    } else {
        lastResponse = response;
        if (responseCallback) {
            responseCallback(response);
        }
    }
}

void ATHandler::RecordTraffic(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                              const std::string& payload) {
    if (esp) {
        esp->RecordTraffic(dir, ch, linkId, tag, payload);
    }
}

bool ATHandler::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, commandBuffer);
    ISerializable::Serialize(out, echoEnabled);
    bool mux = muxEnabled.load();
    ISerializable::Serialize(out, mux);
    ISerializable::Serialize(out, wifiConnected);
    ISerializable::Serialize(out, cwMode);
    ISerializable::Serialize(out, connectedSSID);
    return out.good();
}

bool ATHandler::LoadState(std::istream& inStream) {
    ISerializable::Deserialize(inStream, commandBuffer);
    ISerializable::Deserialize(inStream, echoEnabled);
    bool mux = false;
    ISerializable::Deserialize(inStream, mux);
    muxEnabled.store(mux);
    ISerializable::Deserialize(inStream, wifiConnected);
    ISerializable::Deserialize(inStream, cwMode);
    ISerializable::Deserialize(inStream, connectedSSID);
    currentState = ATState::Idle;
    return inStream.good();
}

void ATHandler::HandleWrite(Byte data) {
    if (currentState == ATState::WaitingCIPSENDData) {
        cipsendBuffer += static_cast<char>(data);
        cipsendReceived++;
        if (cipsendReceived >= cipsendLength) {
            RecordTraffic(TrafficDirection::TX, TrafficChannel::DataPayload, cipsendLinkId,
                          "CIPSEND (" + std::to_string(cipsendLength) + " B)", cipsendBuffer);
            if (networkLoop) {
                networkLoop->SendDataOnLink(cipsendLinkId, cipsendBuffer);
            }
            EnqueueResponse("\r\nSEND OK\r\n");
            currentState = ATState::Idle;
            cipsendBuffer.clear();
        }
    } else {
        if (data == '\n') {
            ProcessCurrentCommandBuffer();
        } else if (data != '\r') {
            commandBuffer += static_cast<char>(data);
        }
    }
}

void ATHandler::ProcessCommand(const std::string& cmd) {
    commandBuffer = cmd;
    while (!commandBuffer.empty() && (commandBuffer.back() == '\r' || commandBuffer.back() == '\n')) {
        commandBuffer.pop_back();
    }
    ProcessCurrentCommandBuffer();
}

void ATHandler::ProcessCurrentCommandBuffer() {
    if (commandBuffer.empty()) {
        return;
    }

    std::string original = commandBuffer;
    std::string upper = commandBuffer;
    std::ranges::transform(upper, upper.begin(), [](unsigned char chr) { return std::toupper(chr); });

    RecordTraffic(TrafficDirection::TX, TrafficChannel::ATCommand, -1, original, original);

    if (echoEnabled) {
        EnqueueResponse(original + "\r\n");
    }

    if (upper == "AT") {
        EnqueueResponse("\r\nOK\r\n");
    } else if (upper.starts_with("AT+")) {
        DispatchATCommand(upper, original);
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }

    commandBuffer.clear();
}

void ATHandler::DispatchATCommand(const std::string& upper, const std::string& original) {
    if (upper == "AT+GMR") {
        HandleGMR();
    } else if (upper.starts_with("AT+CWMODE")) {
        HandleCWMode(upper);
    } else if (upper.starts_with("AT+CWJAP")) {
        HandleCWJAP(original);
    } else if (upper == "AT+CWLAP") {
        HandleCWLAP();
    } else if (upper == "AT+CWQAP") {
        HandleCWQAP();
    } else if (upper == "AT+CIFSR") {
        HandleCIFSR();
    } else if (upper.starts_with("AT+CIPSTART")) {
        HandleCIPSTART(original);
    } else if (upper.starts_with("AT+CIPSEND")) {
        HandleCIPSEND(upper);
    } else if (upper.starts_with("AT+CIPCLOSE")) {
        HandleCIPCLOSE(upper);
    } else if (upper.starts_with("AT+CIPMUX")) {
        HandleCIPMUX(upper);
    } else if (upper.starts_with("AT+CIPSTATUS")) {
        HandleCIPSTATUS();
    } else if (upper.starts_with("AT+CIPSERVER")) {
        HandleCIPSERVER(upper);
    } else if (upper.starts_with("AT+HTTPCLIENT")) {
        HandleHTTPCLIENT(original);
    } else if (upper.starts_with("AT+PING")) {
        HandlePing(original);
    } else if (upper == "AT+RST") {
        if (esp) {
            esp->Reset();
        } else {
            Reset();
        }
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ATHandler::HandleCWMode(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\n+CWMODE:" + std::to_string(static_cast<int>(cwMode)) + "\r\nOK\r\n");
        return;
    }

    int mode = 0;
    try {
        mode = std::stoi(cmd.substr(pos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    cwMode = static_cast<CWMode>(mode);
    EnqueueResponse("\r\nOK\r\n");
}

void ATHandler::HandleCWJAP(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        if (wifiConnected) {
            EnqueueResponse("\r\n+CWJAP:\"" + connectedSSID + "\"\r\nOK\r\n");
        } else {
            EnqueueResponse("\r\nNo AP\r\nOK\r\n");
        }
        return;
    }

    size_t start = pos + 1;
    std::string ssid = ParseQuotedParam(cmd, start);

    if (ssid.empty()) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    connectedSSID = ssid;
    wifiConnected = true;
    EnqueueResponse("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
}

void ATHandler::HandleCWLAP() {
    EnqueueResponse("\r\n+CWLAP:(3,\"Simulator_AP\",-40,\"00:11:22:33:44:55\",1)\r\nOK\r\n");
}

void ATHandler::HandleCWQAP() {
    wifiConnected = false;
    connectedSSID.clear();
    EnqueueResponse("\r\nOK\r\n");
}

void ATHandler::HandleCIFSR() {
    if (!wifiConnected) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string localIp = networkLoop ? networkLoop->GetLocalIP() : "127.0.0.1";
    EnqueueResponse("\r\n+CIFSR:STAIP,\"" + localIp + "\"\r\n+CIFSR:STAMAC,\"00:11:22:33:44:55\"\r\nOK\r\n");
}

void ATHandler::HandleCIPSTART(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(pos + 1);
    size_t ppos = 0;

    int linkId = -1;
    std::string type;
    std::string host;
    int port = 0;

    try {
        if (muxEnabled.load(std::memory_order_relaxed)) {
            linkId = ParseIntParam(params, ppos);
            if (ppos < params.size() && params[ppos] == ',') {
                ppos++;
            }
        } else {
            linkId = 0;
        }

        type = ParseQuotedParam(params, ppos);
        if (ppos < params.size() && params[ppos] == ',') {
            ppos++;
        }
        host = ParseQuotedParam(params, ppos);
        if (ppos < params.size() && params[ppos] == ',') {
            ppos++;
        }
        port = ParseIntParam(params, ppos);

        if (linkId < 0 || linkId >= kMaxConnections) {
            EnqueueResponse("\r\nID ERROR\r\nERROR\r\n");
            return;
        }

        if (host.empty()) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }

        PerformConnection(linkId, type, host, port, params, ppos);

    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ATHandler::PerformConnection(int linkId, const std::string& type, const std::string& host, int port,
                                  const std::string& params, size_t pos) {
    if (!networkLoop) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (type == "TCP") {
        networkLoop->ConnectTCP(linkId, host, port);
    } else if (type == "UDP") {
        int localPort = 0;
        int mode = 0;
        if (pos < params.size() && params[pos] == ',') {
            pos++;
            localPort = ParseIntParam(params, pos);
            if (pos < params.size() && params[pos] == ',') {
                pos++;
                mode = ParseIntParam(params, pos);
            }
        }
        networkLoop->ConnectUDP(linkId, host, port, localPort, mode);
    } else if (type == "SSL" || type == "HTTPS") {
#ifndef TARGET_WASM
        networkLoop->ConnectSSL(linkId, host, port);
#else
        EnqueueResponse("\r\nSSL/HTTPS not supported in WASM\r\nERROR\r\n");
#endif
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ATHandler::HandleCIPSEND(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(pos + 1);
    size_t ppos = 0;

    if (muxEnabled.load(std::memory_order_relaxed)) {
        cipsendLinkId = ParseIntParam(params, ppos);
        if (ppos < params.size() && params[ppos] == ',') {
            ppos++;
        }
        cipsendLength = ParseIntParam(params, ppos);
    } else {
        cipsendLinkId = 0;
        cipsendLength = ParseIntParam(params, ppos);
    }

    if (cipsendLength <= 0) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (cipsendLinkId < 0 || cipsendLinkId >= kMaxConnections ||
        !networkLoop || !networkLoop->IsConnectionActive(cipsendLinkId)) {
        EnqueueResponse("\r\nlink is not valid\r\nERROR\r\n");
        return;
    }

    cipsendReceived = 0;
    cipsendBuffer.clear();
    currentState = ATState::WaitingCIPSENDData;
    EnqueueResponse("\r\nOK\r\n> ");
}

void ATHandler::HandleCIPCLOSE(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        if (muxEnabled.load(std::memory_order_relaxed)) {
            EnqueueResponse("\r\nERROR\r\n");
        } else {
            if (networkLoop) {
                networkLoop->DisconnectLink(0);
            }
            EnqueueResponse("\r\nCLOSED\r\nOK\r\n");
        }
        return;
    }

    int linkId = -1;
    try {
        linkId = std::stoi(cmd.substr(pos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    if (linkId == 5) {
        if (networkLoop) {
            networkLoop->DisconnectAll();
        }
        EnqueueResponse("\r\nOK\r\n");
    } else if (linkId >= 0 && linkId < kMaxConnections) {
        if (networkLoop) {
            networkLoop->DisconnectLink(linkId);
        }
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ATHandler::HandleCIPMUX(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\n+CIPMUX:" + std::to_string(muxEnabled.load(std::memory_order_relaxed) ? 1 : 0) + "\r\nOK\r\n");
        return;
    }

    int mode = 0;
    try {
        mode = std::stoi(cmd.substr(pos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    if (mode == 1 && networkLoop && networkLoop->IsAnyConnectionActive()) {
        EnqueueResponse("\r\nLink is builded\r\nERROR\r\n");
        return;
    }

    muxEnabled.store(mode == 1, std::memory_order_relaxed);
    EnqueueResponse("\r\nOK\r\n");
}

void ATHandler::HandleCIPSTATUS() {
    std::string resp = "\r\nSTATUS:2\r\n";
    if (networkLoop) {
        for (int i = 0; i < kMaxConnections; ++i) {
            const auto& conn = networkLoop->GetConnection(i);
            if (conn.active) {
                resp += "+CIPSTATUS:" + std::to_string(i) + ",\"" + conn.protocol + "\",\"" + conn.remoteHost + "\"," +
                        std::to_string(conn.remotePort) + ",0\r\n";
            }
        }
    }
    resp += "OK\r\n";
    EnqueueResponse(resp);
}

void ATHandler::HandleCIPSERVER(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    size_t start = pos + 1;
    int mode = ParseIntParam(cmd, start);
    int port = 333;

    if (start < cmd.size() && cmd[start] == ',') {
        start++;
        port = ParseIntParam(cmd, start);
    }

    if (mode == 1) {
        if (networkLoop) {
            networkLoop->StartServer(port);
        } else {
            EnqueueResponse("\r\nOK\r\n");
        }
    } else {
        if (networkLoop) {
            networkLoop->StopServer();
        }
        EnqueueResponse("\r\nOK\r\n");
    }
}

void ATHandler::HandleGMR() {
    EnqueueResponse(
        "\r\nAT version:1.7.4.0(May 11 2020 19:13:04)\r\nSDK version:3.0.4(57c2371)\r\ncompile time:May 27 2020 "
        "10:35:14\r\nBin version(Wroom 02):1.7.4\r\nOK\r\n");
}

void ATHandler::HandlePing(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string host = ParseQuotedParam(cmd, pos);
    if (networkLoop) {
        networkLoop->StartPing(host);
    } else {
        EnqueueResponse("\r\n+71\r\nOK\r\n");
    }
}

std::string ATHandler::ParseQuotedParam(const std::string& cmd, size_t& pos) {
    size_t firstQuote = cmd.find('"', pos);
    if (firstQuote == std::string::npos) {
        return "";
    }
    size_t lastQuote = cmd.find('"', firstQuote + 1);
    if (lastQuote == std::string::npos) {
        return "";
    }
    pos = lastQuote + 1;
    return cmd.substr(firstQuote + 1, lastQuote - firstQuote - 1);
}

int ATHandler::ParseIntParam(const std::string& cmd, size_t& pos) {
    size_t nextComma = cmd.find(',', pos);
    std::string valStr = cmd.substr(pos, nextComma - pos);
    if (nextComma != std::string::npos) {
        pos = nextComma;
    } else {
        pos = cmd.size();
    }
    try {
        return std::stoi(valStr);
    } catch (...) {
        return -1;
    }
}

void ATHandler::HandleHTTPCLIENT(const std::string& cmd) {
    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    size_t ppos = eqPos + 1;
    int opt = ParseIntParam(cmd, ppos);
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
    }
    int contentType = ParseIntParam(cmd, ppos);
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
    }
    std::string url = ParseQuotedParam(cmd, ppos);
    if (url.empty()) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    std::string host;
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
        host = ParseQuotedParam(cmd, ppos);
    }
    std::string path;
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
        path = ParseQuotedParam(cmd, ppos);
    }
    int dataType = 0;
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
        dataType = ParseIntParam(cmd, ppos);
    }
    std::string postData;
    if (ppos < cmd.size() && cmd[ppos] == ',') {
        ppos++;
        postData = ParseQuotedParam(cmd, ppos);
    }
    (void)dataType;

#ifndef TARGET_WASM
    if (networkLoop) {
        networkLoop->StartHttpClientRequest(opt, contentType, url, host, path, postData);
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
#else
    EnqueueResponse("\r\nHTTPCLIENT not supported in WASM\r\nERROR\r\n");
#endif
}

}  // namespace Hardware
