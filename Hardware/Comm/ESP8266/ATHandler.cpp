#include "Hardware/Comm/ESP8266.h"

#include <algorithm>

namespace Hardware {

void ESP8266::ProcessCommand() {
    if (commandBuffer.empty()) {
        return;
    }

    std::string original = commandBuffer;
    std::string upper = commandBuffer;
    std::ranges::transform(upper, upper.begin(), [](unsigned char chr) { return std::toupper(chr); });

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

void ESP8266::DispatchATCommand(const std::string& upper, const std::string& original) {
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
    } else if (upper == "AT+CIPSTATUS") {
        HandleCIPSTATUS();
    } else if (upper.starts_with("AT+CIPSERVER")) {
        HandleCIPSERVER(upper);
    } else if (upper.starts_with("AT+PING")) {
        HandlePing(original);
    } else if (upper == "AT+RST") {
        Reset();
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::HandleCWMode(const std::string& cmd) {
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

void ESP8266::HandleCWJAP(const std::string& cmd) {
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
    // std::string pwd = ParseQuotedParam(cmd, start); // Password ignored in simulation

    connectedSSID = ssid;
    wifiConnected = true;
    EnqueueResponse("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
}

void ESP8266::HandleCWLAP() {
    EnqueueResponse("\r\n+CWLAP:(3,\"Simulator_AP\",-40,\"00:11:22:33:44:55\",1)\r\nOK\r\n");
}

void ESP8266::HandleCWQAP() {
    wifiConnected = false;
    connectedSSID = "";
    EnqueueResponse("\r\nOK\r\n");
}

void ESP8266::HandleCIFSR() {
    if (!wifiConnected) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    EnqueueResponse("\r\n+CIFSR:STAIP,\"192.168.1.100\"\r\n+CIFSR:STAMAC,\"00:11:22:33:44:55\"\r\nOK\r\n");
}

void ESP8266::HandleCIPSTART(const std::string& cmd) {
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
        if (muxEnabled) {
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

        PerformConnection(linkId, type, host, port, params, ppos);

    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::PerformConnection(int linkId, const std::string& type, const std::string& host, int port, const std::string& params, size_t pos) {
    if (type == "TCP") {
        ConnectTCP(linkId, host, port);
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
        ConnectUDP(linkId, host, port, localPort, mode);
    } else if (type == "SSL") {
#ifndef TARGET_WASM
        ConnectSSL(linkId, host, port);
#else
        EnqueueResponse("\r\nSSL not supported in WASM\r\nERROR\r\n");
#endif
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::HandleCIPSEND(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    std::string params = cmd.substr(pos + 1);
    size_t ppos = 0;

    if (muxEnabled) {
        cipsendLinkId = ParseIntParam(params, ppos);
        if (ppos < params.size() && params[ppos] == ',') {
            ppos++;
        }
        cipsendLength = ParseIntParam(params, ppos);
    } else {
        cipsendLinkId = 0;
        cipsendLength = ParseIntParam(params, ppos);
    }

    if (cipsendLinkId < 0 || cipsendLinkId >= kMaxConnections || !connections.at(cipsendLinkId).active) {
        EnqueueResponse("\r\nlink is not valid\r\nERROR\r\n");
        return;
    }

    cipsendReceived = 0;
    cipsendBuffer.clear();
    currentState = ATState::WaitingCIPSENDData;
    EnqueueResponse("\r\nOK\r\n> ");
}

void ESP8266::HandleCIPCLOSE(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        if (muxEnabled) {
            EnqueueResponse("\r\nERROR\r\n");
        } else {
            DisconnectLink(0);
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
        DisconnectAll();
        EnqueueResponse("\r\nOK\r\n");
    } else if (linkId >= 0 && linkId < kMaxConnections) {
        DisconnectLink(linkId);
        EnqueueResponse("\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::HandleCIPMUX(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\n+CIPMUX:" + std::to_string(muxEnabled ? 1 : 0) + "\r\nOK\r\n");
        return;
    }

    int mode = 0;
    try {
        mode = std::stoi(cmd.substr(pos + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    if (mode == 1 && IsAnyConnectionActive()) {
        EnqueueResponse("\r\nLink is builded\r\nERROR\r\n");
        return;
    }

    muxEnabled = (mode == 1);
    EnqueueResponse("\r\nOK\r\n");
}

void ESP8266::HandleCIPSTATUS() {
    std::string resp = "\r\nSTATUS:2\r\n"; // 2 = Got IP
    for (int i = 0; i < kMaxConnections; ++i) {
        auto& conn = connections.at(i);
        if (conn.active) {
            resp += "+CIPSTATUS:" + std::to_string(i) + ",\"" + conn.protocol + "\",\"" + conn.remoteHost + "\"," + std::to_string(conn.remotePort) + ",0\r\n";
        }
    }
    resp += "OK\r\n";
    EnqueueResponse(resp);
}

void ESP8266::HandleCIPSERVER(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    size_t start = pos + 1;
    int mode = ParseIntParam(cmd, start);
    int port = 333; // Default

    if (start < cmd.size() && cmd[start] == ',') {
        start++;
        port = ParseIntParam(cmd, start);
    }

    if (mode == 1) {
        StartServer(port);
    } else {
        StopServer();
        EnqueueResponse("\r\nOK\r\n");
    }
}

void ESP8266::HandleGMR() {
    EnqueueResponse("\r\nAT version:1.7.4.0(May 11 2020 19:13:04)\r\nSDK version:3.0.4(57c2371)\r\ncompile time:May 27 2020 10:35:14\r\nBin version(Wroom 02):1.7.4\r\nOK\r\n");
}

void ESP8266::HandlePing(const std::string& cmd) {
    size_t pos = cmd.find('=');
    if (pos == std::string::npos) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (pingThread.joinable()) {
        stopPing.store(true);
        pingThread.join();
    }
    stopPing.store(false);

    std::string host = ParseQuotedParam(cmd, pos);
    pingThread = std::thread(&ESP8266::PingTask, this, host);
}

void ESP8266::PingTask(const std::string& host) {
    int loops = 50;
    while (loops-- > 0) {
        if (stopPing.load()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EnqueueResponse("\r\n+71\r\nOK\r\n");
}

std::string ESP8266::ParseQuotedParam(const std::string& cmd, size_t& pos) {
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

int ESP8266::ParseIntParam(const std::string& cmd, size_t& pos) {
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

void ESP8266::EnqueueLinkClosedResponse(int linkId) {
    if (muxEnabled) {
        EnqueueResponse(std::to_string(linkId) + ",CLOSED\r\n");
    } else {
        EnqueueResponse("CLOSED\r\n");
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

}  // namespace Hardware
