#include "Hardware/Comm/ESP8266.h"

#include <iostream>
#include <cstring>
#include <array>

#include <httplib.h>



namespace Hardware {

ESP8266::ESP8266() : statusReg(0), cmdReg(0), ctrlReg(0), connected(false), threadRunning(false) {
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
                    if (tcpSocket && tcpSocket->is_open()) {
                        asio::error_code errCode;
                        asio::write(*tcpSocket, asio::buffer(&chr, 1), errCode);
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
        if (tcpSocket && tcpSocket->is_open()) {
            commandBuffer += "\r\n";
            asio::error_code errCode;
            asio::write(*tcpSocket, asio::buffer(commandBuffer), errCode);
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
    
    tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);
    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code errCode;
    auto endpoints = resolver.resolve(host, std::to_string(port), errCode);
    if (errCode) {
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }
    
    asio::connect(*tcpSocket, endpoints, errCode);
    if (errCode) {
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
        
        if (tcpSocket && tcpSocket->is_open()) {
            asio::error_code errCode;
            errCode = tcpSocket->shutdown(asio::ip::tcp::socket::shutdown_both, errCode);
            errCode = tcpSocket->close(errCode);
        }
        tcpSocket.reset();
        
        if (rxThread.joinable()) {
            rxThread.join();
        }
    }
}

void ESP8266::RxLoop() {
    std::array<Byte, 1024> buffer{};
    while (threadRunning) {
        asio::error_code errCode;
        size_t num_bytes = tcpSocket->read_some(asio::buffer(buffer), errCode);
        if (!errCode && num_bytes > 0) {
            std::lock_guard<std::mutex> lock(rxMutex);
            for (size_t i = 0; i < num_bytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg |= 0x80; // Signal IRQ/Data valid
        } else {
            // Connection closed by remote or error
            if (errCode == asio::error::would_block || errCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
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
