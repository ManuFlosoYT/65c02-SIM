#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <chrono>

namespace Hardware {

ESP8266::ESP8266()
    : statusReg(0),
      ctrlReg(0),
      networkLoop(*this),
      atHandler(this) {
    atHandler.SetNetworkLoop(&networkLoop);
    Reset();
}

ESP8266::~ESP8266() = default;

void ESP8266::Reset() {
    networkLoop.Reset();
    statusReg = 0;
    ctrlReg = 0;
    {
        std::lock_guard<std::mutex> lock(rxMutex);
        while (!rxQueue.empty()) {
            rxQueue.pop();
        }
    }
    atHandler.Reset();
}

Byte ESP8266::Read(Word address) {
    switch (address & 0x03) {
        case 0: {
            std::lock_guard<std::mutex> lock(rxMutex);
            if (rxQueue.empty()) {
                statusReg.fetch_and(0x7F);
                return 0;
            }
            Byte data = rxQueue.front();
            rxQueue.pop();
            if (rxQueue.empty()) {
                statusReg.fetch_and(0x7F);
            }
            return data;
        }
        case 1:
            return statusReg.load(std::memory_order_relaxed);
        case 2:
            return ctrlReg;
        default:
            return 0;
    }
}

Byte ESP8266::ReadDirect(Word address) {
    switch (address & 0x03) {
        case 0:
            return 0;
        case 1:
            return statusReg.load(std::memory_order_relaxed);
        case 2:
            return ctrlReg;
        default:
            return 0;
    }
}

void ESP8266::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case 0:
            atHandler.HandleWrite(data);
            break;
        case 2:
            ctrlReg = data;
            if ((data & 0x01) != 0) {
                Reset();
            }
            break;
        default:
            break;
    }
}

std::string ESP8266::GetName() const { return "ESP8266"; }

bool ESP8266::SaveState(std::ostream& out) const {
    Byte sReg = statusReg.load();
    ISerializable::Serialize(out, sReg);
    ISerializable::Serialize(out, ctrlReg);
    atHandler.SaveState(out);
    return out.good();
}

bool ESP8266::LoadState(std::istream& inStream) {
    networkLoop.Reset();

    Byte sReg = 0;
    ISerializable::Deserialize(inStream, sReg);
    statusReg.store(sReg);
    ISerializable::Deserialize(inStream, ctrlReg);
    atHandler.LoadState(inStream);

    return inStream.good();
}

void ESP8266::RecordTraffic(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                            const std::string& payload) {
    if (snifferPaused.load(std::memory_order_relaxed)) {
        return;
    }
    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
    TrafficPacket pkt{nowMs, dir, ch, linkId, tag, payload};
    std::lock_guard<std::mutex> lock(trafficMutex);
    trafficLog.push_back(std::move(pkt));
    if (trafficLog.size() > kMaxTrafficPackets) {
        trafficLog.pop_front();
    }
}

std::vector<TrafficPacket> ESP8266::GetTrafficLog() const {
    std::lock_guard<std::mutex> lock(trafficMutex);
    return {trafficLog.begin(), trafficLog.end()};
}

void ESP8266::ClearTrafficLog() {
    std::lock_guard<std::mutex> lock(trafficMutex);
    trafficLog.clear();
}

void ESP8266::SetSnifferPaused(bool paused) { snifferPaused.store(paused, std::memory_order_relaxed); }

bool ESP8266::IsSnifferPaused() const { return snifferPaused.load(std::memory_order_relaxed); }

void ESP8266::SetRxByteCallback(std::function<void(Byte)> callback) { rxByteCallback = std::move(callback); }

void ESP8266::EnqueueResponse(const std::string& response) {
    RecordTraffic(TrafficDirection::RX, TrafficChannel::ATResponse, -1, "RESPONSE", response);
    std::lock_guard<std::mutex> lock(rxMutex);
    if (rxQueue.size() + response.length() > 65536) {
        return;
    }
    for (char chr : response) {
        rxQueue.push(static_cast<Byte>(chr));
        if (rxByteCallback) {
            rxByteCallback(static_cast<Byte>(chr));
        }
    }
    if (!rxQueue.empty()) {
        statusReg.fetch_or(0x80);
    }
}

void ESP8266::EnqueueIPD(int linkId, const char* data, size_t bytesRead) {
    RecordTraffic(TrafficDirection::RX, TrafficChannel::DataPayload, linkId, "+IPD (" + std::to_string(bytesRead) + " B)",
                  std::string(data, bytesRead));

    std::string header;
    if (IsMuxEnabled()) {
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

void ESP8266::EnqueueLinkClosedResponse(int linkId) {
    if (IsMuxEnabled()) {
        EnqueueResponse(std::to_string(linkId) + ",CLOSED\r\n");
    } else {
        EnqueueResponse("CLOSED\r\n");
    }
}

bool ESP8266::IsRxBufferFull() const {
    std::lock_guard<std::mutex> lock(rxMutex);
    return rxQueue.size() >= 2048;
}

bool ESP8266::IsMuxEnabled() const {
    return atHandler.IsMuxEnabled();
}

}  // namespace Hardware
