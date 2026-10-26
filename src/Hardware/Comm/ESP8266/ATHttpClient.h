#pragma once

#include <memory>
#include <string>
#include <thread>

namespace Hardware {

class ESP8266;

class ATHttpClient {
public:
    explicit ATHttpClient(ESP8266& esp);
    ~ATHttpClient();

    ATHttpClient(const ATHttpClient&) = delete;
    ATHttpClient& operator=(const ATHttpClient&) = delete;
    ATHttpClient(ATHttpClient&&) = delete;
    ATHttpClient& operator=(ATHttpClient&&) = delete;

    void Reset();

#ifndef TARGET_WASM
    void StartHttpClientRequest(int opt, int contentType, const std::string& url,
                                const std::string& host, const std::string& path,
                                const std::string& postData);

    void StartHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData);

    static bool ParseUrlComponents(const std::string& url, std::string& scheme, std::string& host, int& port,
                                   std::string& path);
#endif

private:
#ifndef TARGET_WASM
    void PerformHttpClientRequest(int opt, int contentType, const std::string& url,
                                  const std::string& host, const std::string& path,
                                  const std::string& postData);
    void PerformHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData);
#endif

    ESP8266& esp;
    std::thread httpClientThread;
};

}  // namespace Hardware
