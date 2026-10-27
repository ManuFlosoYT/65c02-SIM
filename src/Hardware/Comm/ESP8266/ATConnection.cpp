#include "Hardware/Comm/ESP8266/ATConnection.h"

#include <exception>

namespace Hardware {

ATConnection::~ATConnection() {
    StopRxThread();
}

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

void ATConnection::Reset() {
    StopRxThread();
    udpRemoteEndpoint = {};
    udpMode = 0;
    isHttps = false;
    protocol.clear();
    remoteHost.clear();
    remotePort = 0;
}

}  // namespace Hardware
