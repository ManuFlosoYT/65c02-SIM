#include "Hardware/Comm/ESP8266.h"

#include <asio.hpp>
#include <chrono>
#include <thread>

namespace Hardware {

void ATConnection::StopRxThread() {
    active = false;
    if (rxThread.joinable()) {
        try {
            if (tcpSocket) {
                asio::error_code closeErr;
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

    conn.StopRxThread();
    conn.udpSocket = std::make_unique<asio::ip::udp::socket>(ioContext);

    asio::error_code openErrCode;
    openErrCode = conn.udpSocket->open(asio::ip::udp::v4(), openErrCode);
    if (openErrCode) {
        conn.udpSocket.reset();
        EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    if (localPort > 0) {
        asio::error_code bindErrCode;
        bindErrCode = conn.udpSocket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), static_cast<unsigned short>(localPort)), bindErrCode);
        (void)bindErrCode;
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

    conn.StopRxThread();
    conn.sslStream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(ioContext, sslContext);

    if (!SSL_set_tlsext_host_name(conn.sslStream->native_handle(), host.c_str())) {  // NOLINT
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

void ESP8266::SendDataOnLink(int linkId, const std::string& data) {
    auto& conn = connections.at(linkId);
    if (!conn.active) {
        return;
    }

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
        // Error recorded in sendErrCode
    }
}

void ESP8266::RxLoopTCP(int linkId) {
    auto& conn = connections.at(linkId);
    std::array<Byte, 1024> buffer{};

    while (conn.active) {
        asio::error_code rxErrCode;
        size_t numBytes = conn.tcpSocket->read_some(asio::buffer(buffer), rxErrCode);
 
        if (numBytes > 0) {
            std::string header = "+IPD," + std::to_string(numBytes) + ":";
            if (muxEnabled) {
                header = "+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char chr : header) {
                rxQueue.push(static_cast<Byte>(chr));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
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
        asio::error_code rxErrCode;
        size_t numBytes = conn.sslStream->read_some(asio::buffer(buffer), rxErrCode);

        if (numBytes > 0) {
            std::string header = "+IPD," + std::to_string(numBytes) + ":";
            if (muxEnabled) {
                header = "+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char chr : header) {
                rxQueue.push(static_cast<Byte>(chr));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
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
        asio::error_code rxErrCode;
        asio::ip::udp::endpoint senderEndpoint;
        size_t numBytes = conn.udpSocket->receive_from(asio::buffer(buffer), senderEndpoint, 0, rxErrCode);

        if (!rxErrCode && numBytes > 0) {
            if (conn.udpMode == 1 || conn.udpMode == 2) {
                conn.udpRemoteEndpoint = senderEndpoint;
            }

            std::string header;
            if (muxEnabled) {
                header = "\r\n+IPD," + std::to_string(linkId) + "," + std::to_string(numBytes) + ":";
            } else {
                header = "\r\n+IPD," + std::to_string(numBytes) + ":";
            }

            std::lock_guard<std::mutex> lock(rxMutex);
            for (char chr : header) {
                rxQueue.push(static_cast<Byte>(chr));
            }
            for (size_t i = 0; i < numBytes; ++i) {
                rxQueue.push(buffer.at(i));
            }
            statusReg.fetch_or(0x80);
        } else {
            if (rxErrCode == asio::error::would_block || rxErrCode == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            conn.active = false;
            break;
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
        int freeId = FindFreeLinkId();
        if (freeId < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto& conn = connections.at(freeId);
        conn.tcpSocket = std::make_unique<asio::ip::tcp::socket>(ioContext);

        asio::error_code acceptErrCode;
        acceptErrCode = serverAcceptor->accept(*conn.tcpSocket, acceptErrCode);

        if (acceptErrCode) {
            conn.tcpSocket.reset();
            if (!serverRunning) {
                break;
            }
            continue;
        }

        asio::error_code epErrCode;
        auto remoteEp = conn.tcpSocket->remote_endpoint(epErrCode);
        conn.active = true;
        conn.protocol = "TCP";
        conn.remoteHost = epErrCode ? "unknown" : remoteEp.address().to_string();
        conn.remotePort = epErrCode ? 0 : static_cast<int>(remoteEp.port());
        conn.rxThread = std::thread(&ESP8266::RxLoopTCP, this, freeId);

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

}  // namespace Hardware
