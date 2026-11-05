#include "Hardware/Comm/ESP8266/ATPingService.h"
#include "Hardware/Comm/ESP8266/ATPingPlatform.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <string>
#include <thread>

namespace Hardware {

namespace Platform = Comm::Platform;

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

    auto timeResult = Platform::ExecutePing(host, stopPing);
    if (stopPing.load()) {
        return;
    }

    if (timeResult.has_value()) {
        esp.EnqueueResponse("\r\n+" + *timeResult + "\r\nOK\r\n");
    } else {
        esp.EnqueueResponse("\r\nERROR\r\n");
    }
}

}  // namespace Hardware
