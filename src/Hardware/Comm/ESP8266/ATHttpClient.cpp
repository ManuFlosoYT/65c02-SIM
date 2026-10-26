#include "Hardware/Comm/ESP8266/ATHttpClient.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

#ifndef TARGET_WASM
#include <httplib.h>
#endif

namespace Hardware {

ATHttpClient::ATHttpClient(ESP8266& espRef) : esp(espRef) {}

ATHttpClient::~ATHttpClient() {
    Reset();
}

void ATHttpClient::Reset() {
#ifndef TARGET_WASM
    if (httpClientThread.joinable()) {
        httpClientThread.join();
    }
#endif
}

#ifndef TARGET_WASM
bool ATHttpClient::ParseUrlComponents(const std::string& url, std::string& scheme, std::string& host, int& port,
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

void ATHttpClient::StartHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData) {
    esp.RecordTraffic(TrafficDirection::NetTX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Req", requestData);
    std::thread(&ATHttpClient::PerformHttpsRequest, this, linkId, host, port, requestData).detach();
}

void ATHttpClient::PerformHttpsRequest(int linkId, const std::string& host, int port, const std::string& requestData) {
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

            esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId,
                              "HTTPS " + std::to_string(res->status), responseData);

            constexpr size_t kChunkSize = 512;
            for (size_t offset = 0; offset < responseData.size(); offset += kChunkSize) {
                size_t len = std::min(kChunkSize, responseData.size() - offset);
                esp.EnqueueIPD(linkId, responseData.data() + offset, len);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        } else {
            std::string errStr = "HTTPS error: " + httplib::to_string(res.error());
            esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Error", errStr);
            std::string failResp = "HTTP/1.1 502 Bad Gateway\r\n\r\n" + errStr;
            esp.EnqueueIPD(linkId, failResp.data(), failResp.size());
        }
    } catch (const std::exception& e) {
        std::string errStr = std::string("HTTPS exception: ") + e.what();
        esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, linkId, "HTTPS Exception", errStr);
        std::string failResp = "HTTP/1.1 500 Internal Server Error\r\n\r\n" + errStr;
        esp.EnqueueIPD(linkId, failResp.data(), failResp.size());
    }
}

void ATHttpClient::StartHttpClientRequest(int opt, int contentType, const std::string& url,
                                          const std::string& host, const std::string& path,
                                          const std::string& postData) {
    if (httpClientThread.joinable()) {
        httpClientThread.join();
    }
    httpClientThread = std::thread(&ATHttpClient::PerformHttpClientRequest, this, opt, contentType, url, host, path, postData);
}

void ATHttpClient::PerformHttpClientRequest(int opt, int contentType, const std::string& url, const std::string& host,
                                            const std::string& path, const std::string& postData) {
    std::string reqScheme = "https";
    std::string reqHost = host;
    int reqPort = 443;
    std::string reqPath = path;

    if (reqHost.empty()) {
        if (!ParseUrlComponents(url, reqScheme, reqHost, reqPort, reqPath)) {
            esp.EnqueueResponse("\r\nERROR\r\n");
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

    esp.RecordTraffic(TrafficDirection::NetTX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Req",
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
            esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1,
                              "HTTPCLIENT " + std::to_string(res->status), res->body);
            std::string resp = "+HTTPCLIENT:" + std::to_string(res->body.size()) + "," + res->body + "\r\n\r\nOK\r\n";
            esp.EnqueueResponse(resp);
        } else {
            std::string errStr = "HTTPCLIENT error: " + httplib::to_string(res.error());
            esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Error", errStr);
            esp.EnqueueResponse("\r\nERROR\r\n");
        }
    } catch (const std::exception& e) {
        std::string errStr = std::string("HTTPCLIENT exception: ") + e.what();
        esp.RecordTraffic(TrafficDirection::NetRX, TrafficChannel::TLS_HTTP, -1, "HTTPCLIENT Exception", errStr);
        esp.EnqueueResponse("\r\nERROR\r\n");
    }
}
#endif

}  // namespace Hardware
