#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <asio.hpp>
#ifndef TARGET_WASM
#include <asio/ssl.hpp>
#endif

namespace Hardware {

struct ATConnection {
    std::unique_ptr<asio::ip::tcp::socket> tcpSocket;
#ifndef TARGET_WASM
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslStream;
#endif
    std::unique_ptr<asio::ip::udp::socket> udpSocket;
    asio::ip::udp::endpoint udpRemoteEndpoint;
    int udpMode{0};

    std::thread rxThread;
    std::recursive_mutex rxThreadMutex;
    std::atomic<bool> active{false};

    bool isHttps{false};
    std::string protocol;
    std::string remoteHost;
    int remotePort{0};

    ATConnection() = default;
    ~ATConnection();

    void StopRxThread();
    void Reset();
};

}  // namespace Hardware
