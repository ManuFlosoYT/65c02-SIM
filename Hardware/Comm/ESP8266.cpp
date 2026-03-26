#include "Hardware/Comm/ESP8266.h"

#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace Hardware {

// ---------------------------------------------------------------------------
// ATConnection
// ---------------------------------------------------------------------------

void ATConnection::StopRxThread() {
    active = false;
    if (tcpSocket && tcpSocket->is_open()) {
        asio::error_code ec;
        tcpSocket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        tcpSocket->close(ec);
    }
#ifndef TARGET_WASM
    if (sslStream) {
        asio::error_code ec;
        sslStream->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        sslStream->lowest_layer().close(ec);
    }
#endif
    if (udpSocket && udpSocket->is_open()) {
        asio::error_code ec;
        udpSocket->close(ec);
    }
    if (rxThread.joinable()) {
        rxThread.join();
    }
    tcpSocket.reset();
#ifndef TARGET_WASM
    sslStream.reset();
#endif
    udpSocket.reset();
    protocol.clear();
    remoteHost.clear();
    remotePort = 0;
    udpMode = 0;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor / Reset
// ---------------------------------------------------------------------------

ESP8266::ESP8266()
    : statusReg(0),
      cmdReg(0),
      ctrlReg(0)
#ifndef TARGET_WASM
      , sslContext(asio::ssl::context::sslv23_client)
#endif
{
#ifndef TARGET_WASM
    sslContext.set_default_verify_paths();
    sslContext.set_verify_mode(asio::ssl::verify_none);
#endif
    Reset();
}

ESP8266::~ESP8266() {
    StopServer();
    DisconnectAll();
}

void ESP8266::Reset() {
    {
        std::lock_guard<std::mutex> lock(rxMutex);
        std::queue<Byte> empty;
        std::swap(rxQueue, empty);
    }

    statusReg.store(static_cast<Byte>(0), std::memory_order_relaxed);
    cmdReg = 0;
    ctrlReg = 0;
    commandBuffer.clear();

    currentState = ATState::Idle;
    cipsendLinkId = 0;
    cipsendLength = 0;
    cipsendReceived = 0;
    cipsendBuffer.clear();

    echoEnabled = true;
    muxEnabled = false;
    wifiConnected = false;
    cwMode = CWMode::Station;
    connectedSSID.clear();

    StopServer();
    DisconnectAll();
}

Byte ESP8266::Read(Word address) {
    switch (address & 0x03) {
        case 0: { // DATA
            std::lock_guard<std::mutex> lock(rxMutex);
            if (rxQueue.empty()) {
                return 0;
            }
            Byte data = rxQueue.front();
            rxQueue.pop();
            if (rxQueue.empty()) {
                statusReg.fetch_and(static_cast<Byte>(0x7F)); // Clear bit 7
            }
            return data;
        }
        case 1: // STATUS
            return statusReg.load(std::memory_order_relaxed);
        case 2: // CMD
            return cmdReg;
        case 3: // CTRL
            return ctrlReg;
        default:
            return 0;
    }
}

std::string ESP8266::GetName() const {
    return "ESP8266";
}

// ---------------------------------------------------------------------------
// Write (memory-mapped register)
// ---------------------------------------------------------------------------

void ESP8266::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case 0: {
            char chr = static_cast<char>(data);

            if (currentState == ATState::WaitingCIPSENDData) {
                if (cipsendReceived == 0 && (chr == '\r' || chr == '\n')) {
                    return;
                }
                cipsendBuffer += chr;
                ++cipsendReceived;
                if (cipsendReceived >= cipsendLength) {
                    SendDataOnLink(cipsendLinkId, cipsendBuffer);
                    EnqueueResponse("\r\nSEND OK\r\n");
                    cipsendBuffer.clear();
                    cipsendReceived = 0;
                    cipsendLength = 0;
                    currentState = ATState::Idle;
                }
                break;
            }

            if (data == '\r' || data == '\n') {
                if (!commandBuffer.empty()) {
                    ProcessCommand();
                    commandBuffer.clear();
                }
            } else {
                commandBuffer += chr;
                if (echoEnabled) {
                    EnqueueResponse(std::string(1, chr));
                }
            }
            break;
        }
        case 1:
            // Status is mostly read-only for the 6502 in this implementation
            break;
        case 2:
            cmdReg = data;
            break;
        case 3:
            ctrlReg = data;
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// ProcessCommand — entry point
// ---------------------------------------------------------------------------

void ESP8266::ProcessCommand() {
    if (commandBuffer == "+++") {
        if (currentState == ATState::PassthroughMode) {
            currentState = ATState::Idle;
        }
        EnqueueResponse("\r\nOK\r\n");
        return;
    }

    std::string upper = commandBuffer;
    for (char& c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    if (!upper.starts_with("AT")) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    DispatchATCommand(upper, commandBuffer);
}

// ---------------------------------------------------------------------------
// AT Command Dispatcher
// ---------------------------------------------------------------------------

void ESP8266::DispatchATCommand(const std::string& upper, const std::string& original) {
    if (upper == "AT") {
        EnqueueResponse("\r\nOK\r\n");
    } else if (upper == "AT+RST") {
        Reset();
        EnqueueResponse("\r\nOK\r\nready\r\n");
    } else if (upper == "ATE0") {
        echoEnabled = false;
        EnqueueResponse("\r\nOK\r\n");
    } else if (upper == "ATE1") {
        echoEnabled = true;
        EnqueueResponse("\r\nOK\r\n");
    } else if (upper == "AT+GMR") {
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
    } else if (upper.starts_with("AT+CIPMUX")) {
        HandleCIPMUX(upper);
    } else if (upper.starts_with("AT+CIPSTART")) {
        HandleCIPSTART(original);
    } else if (upper.starts_with("AT+CIPSEND")) {
        HandleCIPSEND(upper);
    } else if (upper.starts_with("AT+CIPCLOSE")) {
        HandleCIPCLOSE(upper);
    } else if (upper == "AT+CIPSTATUS") {
        HandleCIPSTATUS();
    } else if (upper.starts_with("AT+CIPSERVER")) {
        HandleCIPSERVER(upper);
    } else if (upper.starts_with("AT+CIPSTO=")) {
        int timeout = 0;
        try {
            timeout = std::stoi(upper.substr(10));
        } catch (...) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
        serverTimeout = timeout;
        EnqueueResponse("\r\nOK\r\n");
    } else if (upper.starts_with("AT+PING=")) {
        HandlePing(original);
    } else if (upper.starts_with("AT+UART_CUR") || upper.starts_with("AT+UART_DEF")) {
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

// ---------------------------------------------------------------------------
// WiFi Commands (Simulated)
// ---------------------------------------------------------------------------

void ESP8266::HandleCWMode(const std::string& cmd) {
    if (cmd == "AT+CWMODE?") {
        EnqueueResponse("+CWMODE:" + std::to_string(static_cast<int>(cwMode)) + "\r\n\r\nOK\r\n");
        return;
    }

    if (!cmd.starts_with("AT+CWMODE=")) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    int mode = 0;
    try {
        mode = std::stoi(cmd.substr(10));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (mode < 1 || mode > 3) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    cwMode = static_cast<CWMode>(mode);
    EnqueueResponse("\r\nOK\r\n");
}

void ESP8266::HandleCWJAP(const std::string& cmd) {
    if (cmd.find('?') != std::string::npos) {
        if (wifiConnected) {
            EnqueueResponse("+CWJAP:\"" + connectedSSID + "\"\r\n\r\nOK\r\n");
        } else {
            EnqueueResponse("No AP\r\n\r\nOK\r\n");
        }
        return;
    }

    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    ++pos;
    std::string ssid = ParseQuotedParam(cmd, pos);
    if (ssid.empty()) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::thread([this, ssid]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        wifiConnected = true;
        connectedSSID = ssid;
        EnqueueResponse("WIFI CONNECTED\r\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EnqueueResponse("WIFI GOT IP\r\n\r\nOK\r\n");
    }).detach();
}

void ESP8266::HandleCWLAP() {
    EnqueueResponse(
        "+CWLAP:(3,\"EmulatorNet\",-45,\"aa:bb:cc:dd:ee:ff\",1)\r\n"
        "+CWLAP:(4,\"Neighbor_5G\",-72,\"11:22:33:44:55:66\",6)\r\n"
        "+CWLAP:(2,\"FreeWiFi\",-80,\"de:ad:be:ef:00:01\",11)\r\n"
        "\r\nOK\r\n");
}

void ESP8266::HandleCWQAP() {
    wifiConnected = false;
    connectedSSID.clear();
    EnqueueResponse("\r\nOK\r\n");
}

void ESP8266::HandleCIFSR() {
    EnqueueResponse(
        "+CIFSR:STAIP,\"192.168.1.100\"\r\n"
        "+CIFSR:STAMAC,\"aa:bb:cc:dd:ee:ff\"\r\n"
        "\r\nOK\r\n");
}

// ---------------------------------------------------------------------------
// AT+CIPSTART
// Format: AT+CIPSTART=[<link_id>,]"<type>","<host>",<port>[,<local_port>,<mode>]
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPSTART(const std::string& cmd) {
    if (!wifiConnected) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(eqPos + 1);
    size_t pos = 0;

    int linkId = 0;
    if (muxEnabled) {
        linkId = ParseIntParam(params, pos);
        if (linkId < 0 || linkId >= kMaxConnections) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
        if (pos < params.size() && params[pos] == ',') {
            ++pos;
        }
    }

    if (connections.at(linkId).active) {
        EnqueueResponse("\r\nALREADY CONNECTED\r\n\r\nERROR\r\n");
        return;
    }

    std::string type = ParseQuotedParam(params, pos);
    if (pos < params.size() && params[pos] == ',') {
        ++pos;
    }

    std::string host = ParseQuotedParam(params, pos);
    if (pos < params.size() && params[pos] == ',') {
        ++pos;
    }

    int port = ParseIntParam(params, pos);
    if (host.empty() || port <= 0) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    for (char& c : type) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    if (type == "TCP") {
        std::thread([this, linkId, host, port]() { ConnectTCP(linkId, host, port); }).detach();
    } else if (type == "UDP") {
        int localPort = 0;
        int udpMode = 0;
        if (pos < params.size() && params[pos] == ',') {
            ++pos;
            localPort = ParseIntParam(params, pos);
        }
        if (pos < params.size() && params[pos] == ',') {
            ++pos;
            udpMode = ParseIntParam(params, pos);
        }
        std::thread([this, linkId, host, port, localPort, udpMode]() {
            ConnectUDP(linkId, host, port, localPort, udpMode);
        }).detach();
    } else if (type == "SSL") {
#ifndef TARGET_WASM
        std::thread([this, linkId, host, port]() {
            ConnectSSL(linkId, host, port);
        }).detach();
#else
        EnqueueResponse("\r\nSSL Not Supported\r\nERROR\r\n");
#endif
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

// ---------------------------------------------------------------------------
// AT+CIPSEND
// Format: AT+CIPSEND=[<link_id>,]<length>
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPSEND(const std::string& cmd) {
    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(eqPos + 1);
    int linkId = 0;
    int length = 0;

    size_t commaPos = params.find(',');
    if (muxEnabled && commaPos != std::string::npos) {
        try {
            linkId = std::stoi(params.substr(0, commaPos));
            length = std::stoi(params.substr(commaPos + 1));
        } catch (...) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    } else {
        try {
            length = std::stoi(params);
        } catch (...) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    }

    if (linkId < 0 || linkId >= kMaxConnections || !connections.at(linkId).active || length <= 0) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    cipsendLinkId = linkId;
    cipsendLength = length;
    cipsendReceived = 0;
    cipsendBuffer.clear();
    currentState = ATState::WaitingCIPSENDData;

    EnqueueResponse("\r\n> ");
}

// ---------------------------------------------------------------------------
// AT+CIPCLOSE
// Format: AT+CIPCLOSE[=<link_id>]
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPCLOSE(const std::string& cmd) {
    if (cmd == "AT+CIPCLOSE") {
        if (muxEnabled) {
            DisconnectAll();
        } else {
            DisconnectLink(0);
        }
        EnqueueResponse("CLOSED\r\n\r\nOK\r\n");
        return;
    }

    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    int linkId = 0;
    try {
        linkId = std::stoi(cmd.substr(eqPos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (linkId == 5) {
        DisconnectAll();
        EnqueueResponse("\r\nOK\r\n");
        return;
    }

    if (linkId < 0 || linkId >= kMaxConnections) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    DisconnectLink(linkId);
    EnqueueResponse(std::to_string(linkId) + ",CLOSED\r\n\r\nOK\r\n");
}

// ---------------------------------------------------------------------------
// AT+CIPMUX
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPMUX(const std::string& cmd) {
    if (cmd == "AT+CIPMUX?") {
        EnqueueResponse("+CIPMUX:" + std::to_string(muxEnabled ? 1 : 0) + "\r\n\r\nOK\r\n");
        return;
    }

    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    int val = 0;
    try {
        val = std::stoi(cmd.substr(eqPos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (val != 0 && val != 1) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (IsAnyConnectionActive()) {
        EnqueueResponse("\r\nlink is builded\r\n\r\nERROR\r\n");
        return;
    }

    muxEnabled = (val == 1);
    EnqueueResponse("\r\nOK\r\n");
}

// ---------------------------------------------------------------------------
// AT+CIPSTATUS
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPSTATUS() {
    int statusCode = 2;
    if (wifiConnected) {
        statusCode = IsAnyConnectionActive() ? 3 : 2;
    } else {
        statusCode = 5;
    }

    std::string response = "STATUS:" + std::to_string(statusCode) + "\r\n";

    for (int i = 0; i < kMaxConnections; ++i) {
        if (connections.at(i).active) {
            response += "+CIPSTATUS:" + std::to_string(i) + ",\"" +
                        connections.at(i).protocol + "\",\"" +
                        connections.at(i).remoteHost + "\"," +
                        std::to_string(connections.at(i).remotePort) + ",0\r\n";
        }
    }

    response += "\r\nOK\r\n";
    EnqueueResponse(response);
}

// ---------------------------------------------------------------------------
// AT+CIPSERVER
// Format: AT+CIPSERVER=<mode>[,<port>]
// ---------------------------------------------------------------------------

void ESP8266::HandleCIPSERVER(const std::string& cmd) {
    size_t eqPos = cmd.find('=');
    if (eqPos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(eqPos + 1);
    size_t commaPos = params.find(',');

    int mode = 0;
    int port = 333;  // NOLINT(readability-magic-numbers)
    try {
        mode = std::stoi(params.substr(0, commaPos));
        if (commaPos != std::string::npos) {
            port = std::stoi(params.substr(commaPos + 1));
        }
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (mode == 1) {
        if (!muxEnabled) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
        std::thread([this, port]() { StartServer(port); }).detach();
    } else if (mode == 0) {
        StopServer();
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

// ---------------------------------------------------------------------------
// System Commands
// ---------------------------------------------------------------------------

void ESP8266::HandleGMR() {
    EnqueueResponse(
        "AT version:2.2.0.0(Sep  4 2020 16:16:36)\r\n"
        "SDK version:v3.4-22-g967752e2\r\n"
        "compile time(emu):65c02-SIM\r\n"
        "\r\nOK\r\n");
}

void ESP8266::HandlePing(const std::string& cmd) {
    if (!wifiConnected) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    size_t firstQuote = cmd.find('"');
    if (firstQuote == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    size_t lastQuote = cmd.rfind('"');
    if (lastQuote == std::string::npos || lastQuote == firstQuote) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string host = cmd.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    std::thread([this, host]() { PingTask(host); }).detach();
}

void ESP8266::PingTask(const std::string& host) {
    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code ec;

    auto start = std::chrono::steady_clock::now();
    resolver.resolve(host, "80", ec);
    auto end = std::chrono::steady_clock::now();

    if (ec) {
        EnqueueResponse("+timeout\r\n\r\nERROR\r\n");
        return;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EnqueueResponse("+" + std::to_string(elapsed) + "\r\n\r\nOK\r\n");
}

// ---------------------------------------------------------------------------
// TCP Connection
// ---------------------------------------------------------------------------

void ESP8266::ConnectTCP(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    conn.tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);
    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code ec;

    auto endpoints = resolver.resolve(host, std::to_string(port), ec);
    if (ec) {
        conn.tcpSocket.reset();
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::connect(*conn.tcpSocket, endpoints, ec);
    if (ec) {
        conn.tcpSocket.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.active = true;
    conn.protocol = "TCP";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ESP8266::RxLoopTCP, this, linkId);

    if (muxEnabled) {
        EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}

// ---------------------------------------------------------------------------
// UDP Connection
// ---------------------------------------------------------------------------

void ESP8266::ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode) {
    auto& conn = connections.at(linkId);

    asio::ip::udp::resolver resolver(ioContext);
    asio::error_code ec;

    auto endpoints = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port), ec);
    if (ec || endpoints.empty()) {
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    conn.udpRemoteEndpoint = *endpoints.begin();
    conn.udpMode = mode;

    conn.udpSocket = std::make_unique<asio::ip::udp::socket>(ioContext, asio::ip::udp::v4());
    if (localPort > 0) {
        conn.udpSocket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), static_cast<unsigned short>(localPort)), ec);
        if (ec) {
            conn.udpSocket.reset();
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    }

    conn.active = true;
    conn.protocol = "UDP";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ESP8266::RxLoopUDP, this, linkId);

    if (muxEnabled) {
        EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}

// ---------------------------------------------------------------------------
// SSL Connection
// ---------------------------------------------------------------------------

#ifndef TARGET_WASM
void ESP8266::ConnectSSL(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    conn.sslStream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(ioContext, sslContext);

    if (!SSL_set_tlsext_host_name(conn.sslStream->native_handle(), host.c_str())) {  // NOLINT
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code ec;

    auto endpoints = resolver.resolve(host, std::to_string(port), ec);
    if (ec) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::connect(conn.sslStream->lowest_layer(), endpoints, ec);
    if (ec) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.sslStream->handshake(asio::ssl::stream_base::client, ec);
    if (ec) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.active = true;
    conn.protocol = "SSL";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ESP8266::RxLoopSSL, this, linkId);

    if (muxEnabled) {
        EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}
#endif

// ---------------------------------------------------------------------------
// Disconnect
// ---------------------------------------------------------------------------

void ESP8266::DisconnectLink(int linkId) {
    if (linkId >= 0 && linkId < kMaxConnections) {
        connections.at(linkId).StopRxThread();
    }
}

void ESP8266::DisconnectAll() {
    for (int i = 0; i < kMaxConnections; ++i) {
        connections.at(i).StopRxThread();
    }
}

// ---------------------------------------------------------------------------
// SendDataOnLink
// ---------------------------------------------------------------------------

void ESP8266::SendDataOnLink(int linkId, const std::string& data) {
    auto& conn = connections.at(linkId);
    if (!conn.active) {
        return;
    }

    asio::error_code ec;

    if (conn.tcpSocket && conn.tcpSocket->is_open()) {
        asio::write(*conn.tcpSocket, asio::buffer(data), ec);
#ifndef TARGET_WASM
    } else if (conn.sslStream) {
        asio::write(*conn.sslStream, asio::buffer(data), ec);
#endif
    } else if (conn.udpSocket && conn.udpSocket->is_open()) {
        conn.udpSocket->send_to(asio::buffer(data), conn.udpRemoteEndpoint, 0, ec);
    }

    if (ec) {
    }
}

// ---------------------------------------------------------------------------
// Receive Loops (run on dedicated threads)
// ---------------------------------------------------------------------------

void ESP8266::RxLoopTCP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        asio::error_code ec;
        size_t numBytes = conn.tcpSocket->read_some(asio::buffer(buffer), ec);
 
        if (numBytes > 0) {
            std::string header = "+IPD," + std::to_string(numBytes) + ":";
            if (muxEnabled) {
                header = "+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char c : header) {
                rxQueue.push(static_cast<Byte>(c));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
        }

        if (ec) {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            if (muxEnabled) {
                EnqueueResponse(std::to_string(linkId) + ",CLOSED\r\n");
            } else {
                EnqueueResponse("CLOSED\r\n");
            }
            break;
        }
    }
}

#ifndef TARGET_WASM
void ESP8266::RxLoopSSL(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        asio::error_code ec;
        size_t numBytes = conn.sslStream->read_some(asio::buffer(buffer), ec);

        if (numBytes > 0) {
            std::string header = "+IPD," + std::to_string(numBytes) + ":";
            if (muxEnabled) {
                header = "+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char c : header) {
                rxQueue.push(static_cast<Byte>(c));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
        } else {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            if (muxEnabled) {
                EnqueueResponse(std::to_string(linkId) + ",CLOSED\r\n");
            } else {
                EnqueueResponse("CLOSED\r\n");
            }
            break;
        }
    }
}
#endif

void ESP8266::RxLoopUDP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        asio::error_code ec;
        asio::ip::udp::endpoint senderEndpoint;
        size_t numBytes = conn.udpSocket->receive_from(asio::buffer(buffer), senderEndpoint, 0, ec);

        if (!ec && numBytes > 0) {
            if (conn.udpMode == 2) {
                conn.udpRemoteEndpoint = senderEndpoint;
            } else if (conn.udpMode == 1) {
                conn.udpRemoteEndpoint = senderEndpoint;
            }

            std::string header;
            if (muxEnabled) {
                header = "\r\n+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            } else {
                header = "\r\n+IPD," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char c : header) {
                rxQueue.push(static_cast<Byte>(c));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
        } else {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Server Mode
// ---------------------------------------------------------------------------

void ESP8266::StartServer(int port) {
    StopServer();

    asio::error_code ec;
    serverAcceptor = std::make_unique<asio::ip::tcp::acceptor>(ioContext);

    serverAcceptor->open(asio::ip::tcp::v4(), ec);
    if (ec) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->set_option(asio::socket_base::reuse_address(true));

    serverAcceptor->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), static_cast<unsigned short>(port)), ec);
    if (ec) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverRunning = true;
    EnqueueResponse("\r\nOK\r\n");
    acceptThread = std::thread(&ESP8266::AcceptLoop, this);
}

void ESP8266::StopServer() {
    serverRunning = false;
    if (serverAcceptor && serverAcceptor->is_open()) {
        asio::error_code ec;
        serverAcceptor->close(ec);
    }
    if (acceptThread.joinable()) {
        acceptThread.join();
    }
    serverAcceptor.reset();
}

void ESP8266::AcceptLoop() {
    while (serverRunning) {
        int freeId = FindFreeLinkId();
        if (freeId < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto& conn = connections.at(freeId);
        conn.tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);

        asio::error_code ec;
        serverAcceptor->accept(*conn.tcpSocket, ec);

        if (ec) {
            conn.tcpSocket.reset();
            if (!serverRunning) {
                break;
            }
            continue;
        }

        auto remoteEp = conn.tcpSocket->remote_endpoint(ec);
        conn.active = true;
        conn.protocol = "TCP";
        conn.remoteHost = ec ? "unknown" : remoteEp.address().to_string();
        conn.remotePort = ec ? 0 : static_cast<int>(remoteEp.port());
        conn.rxThread = std::thread(&ESP8266::RxLoopTCP, this, freeId);

        EnqueueResponse(std::to_string(freeId) + ",CONNECT\r\n");
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

int ESP8266::FindFreeLinkId() const {
    for (int i = 0; i < kMaxConnections; ++i) {
        if (!connections.at(i).active) {
            return i;
        }
    }
    return -1;
}

bool ESP8266::IsAnyConnectionActive() const {
    for (int i = 0; i < kMaxConnections; ++i) {
        if (connections.at(i).active) {
            return true;
        }
    }
    return false;
}

std::string ESP8266::ParseQuotedParam(const std::string& cmd, size_t& pos) const {
    size_t start = cmd.find('"', pos);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = cmd.find('"', start + 1);
    if (end == std::string::npos) {
        return "";
    }
    pos = end + 1;
    return cmd.substr(start + 1, end - start - 1);
}

int ESP8266::ParseIntParam(const std::string& cmd, size_t& pos) const {
    size_t start = pos;
    while (pos < cmd.size() && (std::isdigit(static_cast<unsigned char>(cmd[pos])) || cmd[pos] == '-')) {
        ++pos;
    }
    if (pos == start) {
        return -1;
    }
    try {
        return std::stoi(cmd.substr(start, pos - start));
    } catch (...) {
        return -1;
    }
}

void ESP8266::EnqueueResponse(const std::string& response) {
    std::lock_guard<std::mutex> lock(rxMutex);
    for (char chr : response) {
        rxQueue.push(static_cast<Byte>(chr));
    }
    if (!rxQueue.empty()) {
        statusReg.fetch_or(0x80);
    }
}

void ESP8266::Clock() {}

// ---------------------------------------------------------------------------
// State Serialization
// ---------------------------------------------------------------------------

bool ESP8266::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&statusReg), sizeof(statusReg));  // NOLINT
    out.write(reinterpret_cast<const char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    out.write(reinterpret_cast<const char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT

    size_t cmdLen = commandBuffer.length();
    out.write(reinterpret_cast<const char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    out.write(commandBuffer.c_str(), static_cast<std::streamsize>(cmdLen));

    out.write(reinterpret_cast<const char*>(&echoEnabled), sizeof(echoEnabled));      // NOLINT
    out.write(reinterpret_cast<const char*>(&muxEnabled), sizeof(muxEnabled));        // NOLINT
    out.write(reinterpret_cast<const char*>(&wifiConnected), sizeof(wifiConnected));  // NOLINT
    out.write(reinterpret_cast<const char*>(&cwMode), sizeof(cwMode));                // NOLINT

    size_t ssidLen = connectedSSID.length();
    out.write(reinterpret_cast<const char*>(&ssidLen), sizeof(ssidLen));  // NOLINT
    out.write(connectedSSID.c_str(), static_cast<std::streamsize>(ssidLen));

    return out.good();
}

bool ESP8266::LoadState(std::istream& inStream) {
    DisconnectAll();
    StopServer();

    inStream.read(reinterpret_cast<char*>(&statusReg), sizeof(statusReg));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT

    size_t cmdLen = 0;
    inStream.read(reinterpret_cast<char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    commandBuffer.assign(cmdLen, '\0');
    inStream.read(commandBuffer.data(), static_cast<std::streamsize>(cmdLen));

    inStream.read(reinterpret_cast<char*>(&echoEnabled), sizeof(echoEnabled));      // NOLINT
    inStream.read(reinterpret_cast<char*>(&muxEnabled), sizeof(muxEnabled));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&wifiConnected), sizeof(wifiConnected));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&cwMode), sizeof(cwMode));                // NOLINT

    size_t ssidLen = 0;
    inStream.read(reinterpret_cast<char*>(&ssidLen), sizeof(ssidLen));  // NOLINT
    connectedSSID.assign(ssidLen, '\0');
    inStream.read(connectedSSID.data(), static_cast<std::streamsize>(ssidLen));

    currentState = ATState::Idle;
    return inStream.good();
}

}  // namespace Hardware
