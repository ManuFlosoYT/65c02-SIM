#include "Hardware/Comm/ESP8266/NetworkLoop.h"

namespace Hardware {

NetworkLoop::NetworkLoop(ESP8266& espRef)
    : esp(espRef),
      socketManager(espRef),
      httpClient(espRef),
      pingService(espRef) {
    socketManager.SetHttpClient(&httpClient);
}

NetworkLoop::~NetworkLoop() {
    Reset();
}

void NetworkLoop::Reset() {
    pingService.Reset();
    httpClient.Reset();
    socketManager.Reset();
}

void NetworkLoop::ConnectTCP(int linkId, const std::string& host, int port) {
    socketManager.ConnectTCP(linkId, host, port);
}

void NetworkLoop::ConnectUDP(int linkId, const std::string& host, int port, int localPort, int mode) {
    socketManager.ConnectUDP(linkId, host, port, localPort, mode);
}

#ifndef TARGET_WASM
void NetworkLoop::ConnectSSL(int linkId, const std::string& host, int port) {
    socketManager.ConnectSSL(linkId, host, port);
}
#endif

void NetworkLoop::DisconnectLink(int linkId) {
    socketManager.DisconnectLink(linkId);
}

void NetworkLoop::DisconnectAll() {
    socketManager.DisconnectAll();
}

void NetworkLoop::SendDataOnLink(int linkId, const std::string& data) {
    socketManager.SendDataOnLink(linkId, data);
}

void NetworkLoop::StartServer(int port) {
    socketManager.StartServer(port);
}

void NetworkLoop::StopServer() {
    socketManager.StopServer();
}

void NetworkLoop::StartPing(const std::string& host) {
    pingService.StartPing(host);
}

#ifndef TARGET_WASM
void NetworkLoop::StartHttpClientRequest(int opt, int contentType, const std::string& url,
                                         const std::string& host, const std::string& path,
                                         const std::string& postData) {
    httpClient.StartHttpClientRequest(opt, contentType, url, host, path, postData);
}
#endif

int NetworkLoop::FindFreeLinkId() const {
    return socketManager.FindFreeLinkId();
}

bool NetworkLoop::IsAnyConnectionActive() const {
    return socketManager.IsAnyConnectionActive();
}

bool NetworkLoop::IsConnectionActive(int linkId) const {
    return socketManager.IsConnectionActive(linkId);
}

const ATConnection& NetworkLoop::GetConnection(int linkId) const {
    return socketManager.GetConnection(linkId);
}

std::string NetworkLoop::GetLocalIP() {
    return socketManager.GetLocalIP();
}

}  // namespace Hardware
