#include "Hardware/Comm/ESP8266.h"

#include <iostream>
#include <cstring>
#include <array>

#include <httplib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace Hardware {

#ifdef _WIN32
ESP8266::ESP8266() : STATUS(0), CMD(0), CTRL(0), connected(false), sockfd(INVALID_SOCKET), threadRunning(false) {
#else
ESP8266::ESP8266() : statusReg(0), cmdReg(0), ctrlReg(0), connected(false), sockfd(-1), threadRunning(false) {
#endif
    Reset();
}

ESP8266::~ESP8266() {
    DisconnectTCP();
}

void ESP8266::Reset() {
    std::lock_guard<std::mutex> lock(rxMutex);
    std::queue<Byte> empty;
    std::swap(rxQueue, empty);
    
    statusReg = 0;
    cmdReg = 0;
    ctrlReg = 0;
    commandBuffer.clear();
    
    DisconnectTCP();
}

void ESP8266::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case 0: // DATA
            if (data == '\r' || data == '\n') {
                if (!commandBuffer.empty()) {
                    ProcessCommand();
                    commandBuffer.clear();
                }
            } else {
                if (connected && commandBuffer.empty()) {
                    char chr = static_cast<char>(data);
                    if (sockfd != -1) {
                        ::send(sockfd, &chr, 1, 0);
                    }
                } else {
                    commandBuffer += static_cast<char>(data);
                }
            }
            break;
        case 1:
            statusReg = data;
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

void ESP8266::ProcessCommand() {
    if (connected && !commandBuffer.starts_with("AT") && commandBuffer != "+++") {
        if (sockfd != -1) {
            commandBuffer += "\r\n";
            ::send(sockfd, commandBuffer.c_str(), commandBuffer.length(), 0);
        }
        return;
    }

    if (commandBuffer == "+++") {
        EnqueueResponse("\r\nOK\r\n");
        return;
    }

    std::string uppercaseCmd = commandBuffer;
    for (char& chr : uppercaseCmd) {
        chr = static_cast<char>(std::toupper(static_cast<unsigned char>(chr)));
    }

    if (uppercaseCmd == "AT") {
        EnqueueResponse("\r\nOK\r\n");
    } else if (uppercaseCmd == "AT+RST") {
        Reset();
        EnqueueResponse("\r\nOK\r\nready\r\n");
    } else if (uppercaseCmd.starts_with("AT+CIPSTART=")) {
        HandleATCommand(uppercaseCmd);
    } else if (uppercaseCmd.starts_with("AT+HTTPGET=")) {
        HandleHTTPGet(commandBuffer);
    } else if (uppercaseCmd == "AT+CIPCLOSE") {
        DisconnectTCP();
        EnqueueResponse("\r\nCLOSED\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::HandleATCommand(const std::string& cmd) {
    // Basic parsing: AT+CIPSTART="TCP","host",port
    size_t firstQuote = cmd.find('"');
    if (firstQuote == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    size_t secondQuote = cmd.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    
    std::string type = cmd.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    
    size_t thirdQuote = cmd.find('"', secondQuote + 1);
    if (thirdQuote == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    size_t fourthQuote = cmd.find('"', thirdQuote + 1);
    if (fourthQuote == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    
    std::string host = cmd.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);
    
    size_t lastComma = cmd.find(',', fourthQuote + 1);
    if (lastComma == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    
    int port = 0;
    try {
        port = std::stoi(cmd.substr(lastComma + 1));
    } catch (...) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    
    if (type == "TCP") {
        ConnectTCP(host, port);
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::HandleHTTPGet(const std::string& originalCmd) {
    size_t firstQuote = originalCmd.find('"');
    if (firstQuote == std::string::npos) { EnqueueResponse("\r\nERROR\r\n"); return; }
    size_t lastQuote = originalCmd.rfind('"');
    if (lastQuote == std::string::npos || lastQuote == firstQuote) { EnqueueResponse("\r\nERROR\r\n"); return; }
    
    std::string url = originalCmd.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    
    // Spawn thread to handle HTTP get without blocking emulation
    std::thread([this, url]() { HttpGetTask(url); }).detach();
}

void ESP8266::HttpGetTask(std::string url) {
    std::string host;
    std::string path = "/";
    
    size_t protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        url = url.substr(protocolEnd + 3);
    }
    
    size_t pathStart = url.find('/');
    if (pathStart != std::string::npos) {
        host = url.substr(0, pathStart);
        path = url.substr(pathStart);
    } else {
        host = url;
    }
    
    EnqueueResponse("\r\nCONNECT\r\n");
    
    httplib::Client cli("https://" + host);
    cli.set_follow_location(true); // handle redirects
    
    auto res = cli.Get(path);
    if (res && res->status == 200) {
        EnqueueResponse("\r\n" + res->body + "\r\nOK\r\n");
    } else {
        EnqueueResponse("\r\nERROR\r\n");
    }
}

void ESP8266::ConnectTCP(const std::string& host, int port) {
    DisconnectTCP(); // Ensure closed
    
    struct hostent *server = gethostbyname(host.c_str());
    if (server == nullptr) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sockfd == INVALID_SOCKET) {
#else
    if (sockfd < 0) {
#endif
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], static_cast<size_t>(server->h_length)); // NOLINT
    serv_addr.sin_port = htons(static_cast<uint16_t>(port));

    if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) { // NOLINT
        DisconnectTCP();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    
    connected = true;
    threadRunning = true;
    rxThread = std::thread(&ESP8266::RxLoop, this);
    
    EnqueueResponse("\r\nCONNECT\r\n\r\nOK\r\n");
}

void ESP8266::DisconnectTCP() {
    if (connected) {
        threadRunning = false;
        connected = false;
        
#ifdef _WIN32
        if (sockfd != INVALID_SOCKET) {
            closesocket(sockfd);
            sockfd = INVALID_SOCKET;
        }
#else
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
#endif
        
        if (rxThread.joinable()) {
            rxThread.join();
        }
    }
}

void ESP8266::RxLoop() {
    std::array<char, 1024> buffer{};
    while (threadRunning) {
        auto num_bytes = ::recv(sockfd, buffer.data(), sizeof(buffer), 0);
        if (num_bytes > 0) {
            std::lock_guard<std::mutex> lock(rxMutex);
            for (int i = 0; i < num_bytes; ++i) {
                rxQueue.push(static_cast<Byte>(buffer.at(static_cast<size_t>(i))));
            }
            statusReg |= 0x80; // Signal IRQ/Data valid
        } else {
            // Connection closed by remote or error
#ifdef _WIN32
            if (num_bytes < 0) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) continue;
            }
#endif
            threadRunning = false;
            connected = false;
            EnqueueResponse("\r\nCLOSED\r\n");
            break;
        }
    }
}

void ESP8266::EnqueueResponse(const std::string& response) {
    std::lock_guard<std::mutex> lock(rxMutex);
    for (char chr : response) {
        rxQueue.push(static_cast<Byte>(chr));
    }
    if (!rxQueue.empty()) {
        statusReg |= 0x80;
    }
}

void ESP8266::Clock() {
    // Anything periodic
}

bool ESP8266::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&statusReg), sizeof(statusReg));  // NOLINT
    out.write(reinterpret_cast<const char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    out.write(reinterpret_cast<const char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT
    
    size_t cmdLen = commandBuffer.length();
    out.write(reinterpret_cast<const char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    out.write(commandBuffer.c_str(), static_cast<std::streamsize>(cmdLen));
    
    bool conn = connected.load();
    out.write(reinterpret_cast<const char*>(&conn), sizeof(conn));  // NOLINT
    return out.good();
}

bool ESP8266::LoadState(std::istream& inStream) {
    inStream.read(reinterpret_cast<char*>(&statusReg), sizeof(statusReg));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT
    
    size_t cmdLen = 0;
    inStream.read(reinterpret_cast<char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    commandBuffer.assign(cmdLen, '\0');
    inStream.read(commandBuffer.data(), static_cast<std::streamsize>(cmdLen));
    
    bool conn = false;
    inStream.read(reinterpret_cast<char*>(&conn), sizeof(conn));  // NOLINT
    return inStream.good();
}

}  // namespace Hardware
