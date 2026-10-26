#include "Hardware/Comm/ESP8266/ATPingService.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace Hardware {

ATPingService::ATPingService(ESP8266& espRef) : esp(espRef) {}

ATPingService::~ATPingService() {
    Reset();
}

void ATPingService::Reset() {
    StopPing();
}

void ATPingService::StopPing() {
    if (pingThread.joinable()) {
        stopPing.store(true);
        pingThread.join();
    }
}

void ATPingService::StartPing(const std::string& host) {
    StopPing();
    stopPing.store(false);
    pingThread = std::thread(&ATPingService::PingTask, this, host);
}

void ATPingService::PingTask(const std::string& host) {
    bool valid = true;
    for (char c : host) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-')) {
            valid = false;
            break;
        }
    }

    if (!valid || host.empty() || host.front() == '-') {
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

#if defined(__EMSCRIPTEN__)
    int loops = 50;
    while (loops-- > 0) {
        if (stopPing.load()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    esp.EnqueueResponse("\r\n+71\r\nOK\r\n");
#else
    std::string command;
#if defined(_WIN32)
    command = "ping -n 1 -w 2000 " + host;
#else
    command = "ping -c 1 -W 2 " + host;
#endif

#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        esp.EnqueueResponse("\r\nERROR\r\n");
        return;
    }

    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            result += buffer;
        }
        if (stopPing.load()) {
#if defined(_WIN32)
            _pclose(pipe);
#else
            pclose(pipe);
#endif
            return;
        }
    }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    size_t timePos = result.find("time=");
    if (timePos == std::string::npos) {
        timePos = result.find("time<");
    }

    if (timePos != std::string::npos) {
        size_t start = timePos + 5;
        size_t end = result.find("ms", start);
        if (end != std::string::npos) {
            std::string timeStr = result.substr(start, end - start);
            timeStr.erase(0, timeStr.find_first_not_of(" \t"));
            timeStr.erase(timeStr.find_last_not_of(" \t") + 1);
            esp.EnqueueResponse("\r\n+" + timeStr + "\r\nOK\r\n");
            return;
        }
    }
    esp.EnqueueResponse("\r\n+TIMEOUT\r\nERROR\r\n");
#endif
}

}  // namespace Hardware
