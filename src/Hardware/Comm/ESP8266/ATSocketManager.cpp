#include "Hardware/Comm/ESP8266/ATSocketManager.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"
#include "Hardware/Comm/ESP8266/ATHttpClient.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

namespace Hardware {

ATSocketManager::ATSocketManager(ESP8266& espRef)
    : esp(espRef)
#ifndef TARGET_WASM
      ,
      sslContext(asio::ssl::context::sslv23)
#endif
{
#ifndef TARGET_WASM
    sslContext.set_default_verify_paths();
#endif
}

ATSocketManager::~ATSocketManager() {
    Reset();
}

void ATSocketManager::Reset() {
    StopServer();
    DisconnectAll();
}

void ATSocketManager::ConnectTCP(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);

    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;

    auto endpoints = resolver.resolve(host, std::to_string(port), dnsErrCode);
    if (dnsErrCode) {
        conn.tcpSocket.reset();
        esp.EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::error_code connectErrCode;
    asio::connect(*conn.tcpSocket, endpoints, connectErrCode);
    if (connectErrCode) {
        conn.tcpSocket.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.active = true;
    conn.protocol = "TCP";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ATSocketManager::RxLoopTCP, this, linkId);

    if (esp.IsMuxEnabled()) {
        esp.EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        esp.EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}

void ATSocketManager::ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.udpSocket = std::make_unique<asio::ip::udp::socket>(ioContext);

    asio::error_code openErrCode;
    openErrCode = conn.udpSocket->open(asio::ip::udp::v4(), openErrCode);
    if (openErrCode) {
        conn.udpSocket.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.udpSocket->non_blocking(true, openErrCode);
    if (openErrCode) {
        conn.udpSocket.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (localPort > 0) {
        asio::error_code bindErrCode;
        bindErrCode = conn.udpSocket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), static_cast<unsigned short>(localPort)), bindErrCode);
        if (bindErrCode) {
            conn.udpSocket.reset();
            esp.EnqueueResponse("\r\nERROR\r\n");
            return;
        }
    }

    asio::ip::udp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;
    auto endpoints = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port), dnsErrCode);

    if (dnsErrCode) {
        conn.udpSocket.reset();
        esp.EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    conn.udpRemoteEndpoint = *endpoints.begin();
    conn.udpMode = mode;
    conn.active = true;
    conn.protocol = "UDP";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ATSocketManager::RxLoopUDP, this, linkId);

    if (esp.IsMuxEnabled()) {
        esp.EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        esp.EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}

#ifndef TARGET_WASM
void ATSocketManager::ConnectSSL(int linkId, const std::string& host, int port) {
    auto& conn = connections.at(linkId);

    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    conn.StopRxThread();
    conn.sslStream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(ioContext, sslContext);

    if (!SSL_set_tlsext_host_name(conn.sslStream->native_handle(), host.c_str())) {
        conn.sslStream.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code dnsErrCode;

    auto endpoints = resolver.resolve(host, std::to_string(port), dnsErrCode);
    if (dnsErrCode) {
        conn.sslStream.reset();
        esp.EnqueueResponse("\r\nDNS Fail\r\nERROR\r\n");
        return;
    }

    asio::error_code connectErrCode;
    asio::connect(conn.sslStream->lowest_layer(), endpoints, connectErrCode);
    if (connectErrCode) {
        conn.sslStream.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    asio::error_code handshakeErrCode;
    handshakeErrCode = conn.sslStream->handshake(asio::ssl::stream_base::client, handshakeErrCode);
    if (handshakeErrCode) {
        conn.sslStream.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    conn.active = true;
    conn.isHttps = true;
    conn.protocol = "SSL";
    conn.remoteHost = host;
    conn.remotePort = port;
    conn.rxThread = std::thread(&ATSocketManager::RxLoopSSL, this, linkId);

    if (esp.IsMuxEnabled()) {
        esp.EnqueueResponse(std::to_string(linkId) + ",CONNECT\r\n\r\nOK\r\n");
    } else {
        esp.EnqueueResponse("CONNECT\r\n\r\nOK\r\n");
    }
}
#endif

void ATSocketManager::DisconnectLink(int linkId) {
    if (linkId >= 0 && linkId < kMaxConnections) {
        connections.at(linkId).StopRxThread();
    }
}

void ATSocketManager::DisconnectAll() {
    for (int i = 0; i < kMaxConnections; ++i) {
        connections.at(i).StopRxThread();
    }
}

void ATSocketManager::SendDataOnLink(int linkId, const std::string& data) {
    auto& conn = connections.at(linkId);
    std::lock_guard<std::recursive_mutex> lock(conn.rxThreadMutex);
    if (!conn.active) {
        return;
    }

#ifndef TARGET_WASM
    if (conn.isHttps && (data.starts_with("GET ") || data.starts_with("POST ") || data.starts_with("HEAD ") ||
                         data.starts_with("PUT ") || data.starts_with("DELETE ") || data.starts_with("PATCH "))) {
        if (httpClient) {
            httpClient->StartHttpsRequest(linkId, conn.remoteHost, conn.remotePort, data);
        }
        return;
    }
#endif

    esp.RecordTraffic(TrafficDirection::NetTX, TrafficChannel::DataPayload, linkId,
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
        esp.EnqueueResponse("\r\nSEND FAIL\r\n");
    }
}

void ATSocketManager::RxLoopTCP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            if (esp.IsRxBufferFull()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                break;
            }
        }
        if (!conn.active) break;

        asio::error_code rxErrCode;
        size_t numBytes = conn.tcpSocket->read_some(asio::buffer(buffer), rxErrCode);

        if (numBytes > 0) {
            esp.EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        }

        if (rxErrCode) {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            esp.EnqueueLinkClosedResponse(linkId);
            break;
        }
    }
}

#ifndef TARGET_WASM
void ATSocketManager::RxLoopSSL(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            if (esp.IsRxBufferFull()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                break;
            }
        }
        if (!conn.active) break;

        asio::error_code rxErrCode;
        size_t numBytes = conn.sslStream->read_some(asio::buffer(buffer), rxErrCode);

        if (numBytes > 0) {
            esp.EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        } else {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            esp.EnqueueLinkClosedResponse(linkId);
            break;
        }
    }
}
#endif

void ATSocketManager::RxLoopUDP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        while (conn.active) {
            if (esp.IsRxBufferFull()) {
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
            esp.EnqueueLinkClosedResponse(linkId);
            break;
        }

        if (numBytes > 0) {
            if (conn.udpMode == 1 || conn.udpMode == 2) {
                conn.udpRemoteEndpoint = senderEndpoint;
            }
            esp.EnqueueIPD(linkId, reinterpret_cast<const char*>(buffer.data()), numBytes);
        }
    }
}

void ATSocketManager::StartServer(int port) {
    StopServer();

    asio::error_code serverErrCode;
    serverAcceptor = std::make_unique<asio::ip::tcp::acceptor>(ioContext);

    serverErrCode = serverAcceptor->open(asio::ip::tcp::v4(), serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->set_option(asio::socket_base::reuse_address(true));

    serverErrCode = serverAcceptor->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), static_cast<unsigned short>(port)), serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverErrCode = serverAcceptor->listen(asio::socket_base::max_listen_connections, serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverAcceptor->non_blocking(true, serverErrCode);
    if (serverErrCode) {
        serverAcceptor.reset();
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    serverRunning = true;
    esp.EnqueueResponse("\r\nOK\r\n");
    acceptThread = std::thread(&ATSocketManager::AcceptLoop, this);
}

void ATSocketManager::StopServer() {
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

void ATSocketManager::AcceptLoop() {
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
            conn.rxThread = std::thread(&ATSocketManager::RxLoopTCP, this, freeId);
        }

        esp.EnqueueResponse(std::to_string(freeId) + ",CONNECT\r\n");
    }
}

int ATSocketManager::FindFreeLinkId() const {
    for (int i = 0; i < kMaxConnections; ++i) {
        if (!connections.at(i).active) {
            return i;
        }
    }
    return -1;
}

bool ATSocketManager::IsAnyConnectionActive() const {
    for (int i = 0; i < kMaxConnections; ++i) {
        if (connections.at(i).active) {
            return true;
        }
    }
    return false;
}

bool ATSocketManager::IsConnectionActive(int linkId) const {
    if (linkId < 0 || linkId >= kMaxConnections) {
        return false;
    }
    return connections.at(linkId).active;
}

const ATConnection& ATSocketManager::GetConnection(int linkId) const {
    return connections.at(linkId);
}

std::string ATSocketManager::GetLocalIP() {
    std::string localIp = "127.0.0.1";
    try {
        asio::ip::udp::socket socket(ioContext);
        socket.connect(asio::ip::udp::endpoint(asio::ip::make_address("8.8.8.8"), 53));
        localIp = socket.local_endpoint().address().to_string();
    } catch (...) {
        // Fallback to 127.0.0.1 if routing fails
    }
    return localIp;
}

}  // namespace Hardware
