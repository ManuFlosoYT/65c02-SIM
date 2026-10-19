#include "Hardware/Comm/ESP8266.h"

#include <asio.hpp>
#include <chrono>
#include <sstream>
#include <thread>

#ifndef TARGET_WASM
#include <httplib.h>
#endif

namespace Hardware {

void ATConnection::StopRxThread() {
    std::lock_guard<std::recursive_mutex> lock(rxThreadMutex);
    active = false;
    if (rxThread.joinable()) {
        try {
            if (tcpSocket) {
                asio::error_code closeErr;
                tcpSocket->shutdown(asio::ip::tcp::socket::shutdown_both, closeErr);
                closeErr = tcpSocket->close(closeErr);
                (void)closeErr;
            }
            if (udpSocket) {
                asio::error_code closeErr;
                closeErr = udpSocket->close(closeErr);
                (void)closeErr;
            }
#ifndef TARGET_WASM
            if (sslStream) {
                asio::error_code closeErr;
                closeErr = sslStream->lowest_layer().close(closeErr);
                (void)closeErr;
            }
#endif
        } catch (const std::exception& ex) {
            (void)ex;
        }
        rxThread.join();
    }
    tcpSocket.reset();
    udpSocket.reset();
#ifndef TARGET_WASM
    sslStream.reset();
#endif
}

void ESP8266::ConnectTCP(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);

    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;

    auto endpoints = resolver.resolve(host, std::to_string(port), dnsErrCode);
    if (dnsErrCode) {
        conn.tcpSocket.reset();
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::error_code connectErrCode;
    asio::connect(*conn.tcpSocket, endpoints, connectErrCode);
    if (connectErrCode) {
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

void ESP8266::ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.udpSocket = std::make_unique<asio::ip::udp::socket>(ioContext);

    asio::error_code openErrCode;
    openErrCode = conn.udpSocket->open(asio::ip::udp::v4(), openErrCode);
    if (openErrCode) {
        conn.udpSocket.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.udpSocket->non_blocking(true, openErrCode);
    if (openErrCode) {
        conn.udpSocket.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (localPort > 0) {
        asio::error_code bindErrCode;
        bindErrCode = conn.udpSocket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), static_cast<unsigned short>(localPort)), bindErrCode);
        if (bindErrCode) {
            conn.udpSocket.reset();
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    }

    asio::ip::udp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;
    auto endpoints = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port), dnsErrCode);

    if (dnsErrCode) {
        conn.udpSocket.reset();
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    conn.udpRemoteEndpoint = *endpoints.begin();
    conn.udpMode = mode;
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

#ifndef TARGET_WASM
void ESP8266::ConnectSSL(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.sslStream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(ioContext, sslContext);

    if (!SSL_set_tlsext_host_name(conn.sslStream->native_handle(), host.c_str())) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;

    auto endpoints = resolver.resolve(host, std::to_string(port), dnsErrCode);
    if (dnsErrCode) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::error_code connectErrCode;
    asio::connect(conn.sslStream->lowest_layer(), endpoints, connectErrCode);
    if (connectErrCode) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    asio::error_code handshakeErrCode;
    handshakeErrCode = conn.sslStream->handshake(asio::ssl::stream_base::client, handshakeErrCode);
    if (handshakeErrCode) {
        conn.sslStream.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.active = true;
    conn.isHttps = true;
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

void ESP8266::EnqueueIPD(int linkId, const char* data, size_t bytesRead) {
    RecordTraffic(TrafficDirection::RX, TrafficChannel::DataPayload, linkId, "+IPD (" + std::to_string(bytesRead) + " B)",
                  std::string(data, bytesRead));

    std::string header;
    if (muxEnabled.load(std::memory_order_relaxed)) {
        header = "+IPD," + std::to_string(linkId) + "," + std::to_string(bytesRead) + ":";
    } else {
        header = "+IPD," + std::to_string(bytesRead) + ":";
    }

    std::lock_guard<std::mutex> lock(rxMutex);
    for (char chr : header) {
        rxQueue.push(static_cast<Byte>(chr));
        if (rxByteCallback) {
            rxByteCallback(static_cast<Byte>(chr));
        }
    }
    for (size_t i = 0; i < bytesRead; ++i) {
        rxQueue.push(static_cast<Byte>(data[i]));
        if (rxByteCallback) {
            rxByteCallback(static_cast<Byte>(data[i]));
        }
    }
    statusReg.fetch_or(0x80);
}

void ESP8266::SendDataOnLink(int linkId, const std::string& data) {
    auto& conn = connections.at(linkId);
    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    if (!conn.active) {
        return;
    }

#ifndef TARGET_WASM
    if (conn.isHttps && (data.starts_with("GET ") || data.starts_with("POST ") || data.starts_with("HEAD ") ||
                         data.starts_with("PUT ") || data.starts_with("DELETE ") || data.starts_with("PATCH "))) {
        RecordTraffic(TrafficDirection::NetTX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Req", data);
        std::thread(&ESP8266::PerformHttpsRequest, this, linkId, conn.remoteHost, conn.remotePort, data).detach();
        return;
    }
#endif

    RecordTraffic(TrafficDirection::NetTX, TrafficChannel::DataPayload, linkId,
                  "DATA (" + std::to_string(data.size()) + " B)", data);

    asio::error_code sendErrCode;

    if (conn.tcpSocket && conn.tcpSocket->is_open()) {
        asio::write(*conn.tcpSocket, asio::buffer(data), sendErrCode);
#ifndef TARGET_WASM
    } else if (conn.sslStream) {
        asio::write(*conn.sslStream, asio::buffer(data), sendErrCode);
#endif
    } else if (conn.udpSocket && conn.udpSocket->is_open()) {
        conn.udpSocket->send_to(asio::buffer(data), conn.udpRemoteEndpoint, 0, sendErrCode);
    }

    if (sendErrCode) {
        conn.active = false;
        EnqueueResponse("\r\nSEND FAIL\r\n");
    }
}

void ESP8266::RxLoopTCP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            bool full;
            {
                std::lock_guard<std::mutex> lock(rxMutex);
                full = (rxQueue.size() >= 2048);
            }
            if (full) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                break;
            }
        }
        if (!conn.active) break;

        asio::error_code rxErrCode;
        size_t numBytes = conn.tcpSocket->read_some(asio::buffer(buffer), rxErrCode);
 
        if (numBytes > 0) {
            EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        }

        if (rxErrCode) {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            EnqueueLinkClosedResponse(linkId);
            break;
        }
    }
}

#ifndef TARGET_WASM
void ESP8266::RxLoopSSL(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            bool full;
            {
                std::lock_guard<std::mutex> lock(rxMutex);
                full = (rxQueue.size() >= 2048);
            }
            if (full) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                break;
            }
        }
        if (!conn.active) break;

        asio::error_code rxErrCode;
        size_t numBytes = conn.sslStream->read_some(asio::buffer(buffer), rxErrCode);

        if (numBytes > 0) {
            EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        } else {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            EnqueueLinkClosedResponse(linkId);
            break;
        }
    }
}
#endif

void ESP8266::RxLoopUDP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            bool full;
            {
                std::lock_guard<std::mutex> lock(rxMutex);
                full = (rxQueue.size() >= 2048);
            }
            if (full) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                break;
            }
        }
        if (!conn.active) break;

        asio::error_code rxErrCode;
        asio::ip::udp::endpoint senderEndpoint;
        size_t numBytes = conn.udpSocket->receive_from(asio::buffer(buffer), senderEndpoint, 0, rxErrCode);

        if (rxErrCode) {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            conn.active = false;
            EnqueueLinkClosedResponse(linkId);
            break;
        }

        if (numBytes > 0) {
            if (conn.udpMode == 1 || conn.udpMode == 2) {
                conn.udpRemoteEndpoint = senderEndpoint;
            }
            EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        }
    }
}

void ESP8266::StartServer(int port) {
    StopServer();

    asio::error_code serverErrCode;
    serverAcceptor = std::make_unique<asio::ip::tcp::acceptor>(ioContext);

    serverErrCode = serverAcceptor->open(asio::ip::tcp::v4(), serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->set_option(asio::socket_base::reuse_address(true));

    serverErrCode = serverAcceptor->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), static_cast<unsigned short>(port)), serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverErrCode = serverAcceptor->listen(asio::socket_base::max_listen_connections, serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->non_blocking(true, serverErrCode);
    if (serverErrCode) {
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
        asio::error_code serverErrCode;
        serverErrCode = serverAcceptor->close(serverErrCode);
        (void)serverErrCode;
    }
    if (acceptThread.joinable()) {
        acceptThread.join();
    }
    serverAcceptor.reset();
}

void ESP8266::AcceptLoop() {
    while (serverRunning) {
        auto socket = std::make_unique<asio::ip::tcp::socket>(ioContext);
        asio::error_code acceptErrCode;
        serverAcceptor->accept(*socket, acceptErrCode);

        if (acceptErrCode) {
            if (acceptErrCode == asio::error::would_block || acceptErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (!serverRunning) {
                break;
            }
            continue;
        }

        int freeId = FindFreeLinkId();
        if (freeId < 0) {
            asio::error_code closeErr;
            socket->close(closeErr);
            continue;
        }

        auto& conn = connections.at(freeId);
        {
            std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
            conn.StopRxThread();
            conn.tcpSocket = std::move(socket);

            asio::error_code epErrCode;
            auto remoteEp = conn.tcpSocket->remote_endpoint(epErrCode);
            conn.active = true;
            conn.protocol = "TCP";
            conn.remoteHost = epErrCode ? "unknown" : remoteEp.address().to_string();
            conn.remotePort = epErrCode ? 0 : static_cast<int>(remoteEp.port());
            conn.rxThread = std::thread(&ESP8266::RxLoopTCP, this, freeId);
        }

        EnqueueResponse(std::to_string(freeId) + ",CONNECT\r\n");
    }
}

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

#ifndef TARGET_WASM
namespace {

bool ParseUrlComponents(const std::string& url, std::string& scheme, std::string& host, int& port,
                        std::string& path) {
    size_t schemeEnd = url.find("://");
    size_t hostStart = 0;
    if (schemeEnd != std::string::npos) {
        scheme = url.substr(0, schemeEnd);
        hostStart = schemeEnd + 3;
    } else {
        scheme = "https";
        hostStart = 0;
    }

    size_t pathStart = url.find('/', hostStart);
    std::string hostPort;
    if (pathStart != std::string::npos) {
        hostPort = url.substr(hostStart, pathStart - hostStart);
        path = url.substr(pathStart);
    } else {
        hostPort = url.substr(hostStart);
        path = "/";
    }

    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        host = hostPort.substr(0, colon);
        try {
            port = std::stoi(hostPort.substr(colon + 1));
        } catch (...) {
            port = (scheme == "https") ? 443 : 80;
        }
    } else {
        host = hostPort;
        port = (scheme == "https") ? 443 : 80;
    }

    return !host.empty();
}

}  // namespace

void ESP8266::PerformHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData) {
    std::string method = "GET";
    std::string path = "/";
    size_t lineEnd = requestData.find("\r\n");
    if (lineEnd == std::string::npos) {
        lineEnd = requestData.find('\n');
    }
    if (lineEnd != std::string::npos) {
        std::string firstLine = requestData.substr(0, lineEnd);
        std::istringstream iss(firstLine);
        iss >> method >> path;
    }
    if (path.empty()) {
        path = "/";
    }

    httplib::Headers headers;
    std::string body;
    size_t headerEnd = requestData.find("\r\n\r\n");
    if (headerEnd != std::string::npos && lineEnd != std::string::npos) {
        body = requestData.substr(headerEnd + 4);
        std::istringstream hss(requestData.substr(lineEnd + 1, headerEnd - lineEnd));
        std::string hline;
        while (std::getline(hss, hline)) {
            if (!hline.empty() && hline.back() == '\r') {
                hline.pop_back();
            }
            size_t colon = hline.find(':');
            if (colon != std::string::npos) {
                std::string k = hline.substr(0, colon);
                std::string v = hline.substr(colon + 1);
                while (!v.empty() && v.front() == ' ') {
                    v.erase(v.begin());
                }
                if (k != "Host" && k != "Content-Length") {
                    headers.emplace(k, v);
                }
            }
        }
    }

    try {
        std::string base = "https://" + host + ":" + std::to_string(port);
        httplib::Client cli(base);
        cli.set_follow_location(true);
        cli.set_connection_timeout(std::chrono::seconds(5));
        cli.set_read_timeout(std::chrono::seconds(10));
        if (!headers.contains("User-Agent")) {
            headers.emplace("User-Agent", "65C02-SIM/ESP8266");
        }

        httplib::Result res;
        if (method == "POST") {
            std::string ctype = "application/x-www-form-urlencoded";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                ctype = it->second;
            }
            res = cli.Post(path, headers, body, ctype);
        } else if (method == "PUT") {
            std::string ctype = "application/x-www-form-urlencoded";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                ctype = it->second;
            }
            res = cli.Put(path, headers, body, ctype);
        } else if (method == "DELETE") {
            res = cli.Delete(path, headers);
        } else if (method == "HEAD") {
            res = cli.Head(path, headers);
        } else {
            res = cli.Get(path, headers);
        }

        if (res) {
            std::string responseData = "HTTP/1.1 " + std::to_string(res->status) + " " + res->reason + "\r\n";
            for (const auto& [k, v] : res->headers) {
                responseData += k + ": " + v + "\r\n";
            }
            responseData += "\r\n" + res->body;

            RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId,
                          "HTTPS " + std::to_string(res->status), responseData);

            constexpr size_t kChunkSize = 512;
            for (size_t offset = 0; offset < responseData.size(); offset += kChunkSize) {
                size_t len = std::min(kChunkSize, responseData.size() - offset);
                EnqueueIPD(linkId, responseData.data() + offset, len);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        } else {
            std::string errStr = "HTTPS error: " + httplib::to_string(res.error());
            RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Error", errStr);
            std::string failResp = "HTTP/1.1 502 Bad Gateway\r\n\r\n" + errStr;
            EnqueueIPD(linkId, failResp.data(), failResp.size());
        }
    } catch (const std::exception& e) {
        std::string errStr = std::string("HTTPS exception: ") + e.what();
        RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Exception", errStr);
        std::string failResp = "HTTP/1.1 500 Internal Server Error\r\n\r\n" + errStr;
        EnqueueIPD(linkId, failResp.data(), failResp.size());
    }
}

void ESP8266::PerformHttpClientRequest(int opt, int contentType, const std::string& url, const std::string& host,
                                       const std::string& path, const std::string& postData) {
    std::string reqScheme = "https";
    std::string reqHost = host;
    int reqPort = 443;
    std::string reqPath = path;

    if (reqHost.empty()) {
        if (!ParseUrlComponents(url, reqScheme, reqHost, reqPort, reqPath)) {
            EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    } else {
        if (reqPath.empty()) {
            reqPath = "/";
        }
        if (url.rfind("http://", 0) == 0) {
            reqScheme = "http";
            reqPort = 80;
        } else {
            reqScheme = "https";
            reqPort = 443;
        }
    }

    RecordTraffic(TrafficDirection::NetTX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Req",
                  reqScheme + "://" + reqHost + reqPath);

    try {
        std::string base = reqScheme + "://" + reqHost + ":" + std::to_string(reqPort);
        httplib::Client cli(base);
        cli.set_follow_location(true);
        cli.set_connection_timeout(std::chrono::seconds(5));
        cli.set_read_timeout(std::chrono::seconds(10));

        httplib::Headers headers = {{"User-Agent", "65C02-SIM/ESP8266"}};
        std::string ctype = "application/x-www-form-urlencoded";
        if (contentType == 1) {
            ctype = "application/json";
        } else if (contentType == 2) {
            ctype = "multipart/form-data";
        } else if (contentType == 3) {
            ctype = "text/xml";
        }

        httplib::Result res;
        switch (opt) {
            case 1:
                res = cli.Head(reqPath, headers);
                break;
            case 2:
                res = cli.Get(reqPath, headers);
                break;
            case 3:
                res = cli.Post(reqPath, headers, postData, ctype);
                break;
            case 4:
                res = cli.Put(reqPath, headers, postData, ctype);
                break;
            case 5:
                res = cli.Delete(reqPath, headers);
                break;
            default:
                res = cli.Get(reqPath, headers);
                break;
        }

        if (res) {
            RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1,
                          "HTTPCLIENT " + std::to_string(res->status), res->body);
            std::string resp = "+HTTPCLIENT:" + std::to_string(res->body.size()) + "," + res->body + "\r\n\r\nOK\r\n";
            EnqueueResponse(resp);
        } else {
            std::string errStr = "HTTPCLIENT error: " + httplib::to_string(res.error());
            RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Error", errStr);
            EnqueueResponse("\r\nERROR\r\n");
        }
    } catch (const std::exception& e) {
        std::string errStr = std::string("HTTPCLIENT exception: ") + e.what();
        RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Exception", errStr);
        EnqueueResponse("\r\nERROR\r\n");
    }
}
#endif

}  // namespace Hardware
