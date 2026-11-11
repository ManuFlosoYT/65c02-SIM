#include "Hardware/Core/Emulator/SerialIOBridge.h"

#include "Hardware/Comm/ACIA.h"
#include "Hardware/Comm/ESP8266/ESP8266.h"
#include "Hardware/Core/ISerializable.h"

namespace Core {

void SerialIOBridge::Clear() {
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.clear();
    hasInput.store(false, std::memory_order_relaxed);
    baudDelay = 0;
}

void SerialIOBridge::InjectKey(char key) {
    if (key == '\n') {
        key = '\r';
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(key);
    hasInput.store(true, std::memory_order_relaxed);
}

void SerialIOBridge::InjectText(std::string_view text) {
    if (text.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\0') {
            continue;
        }
        if (c == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            inputBuffer.push_back('\r');
        } else if (c == '\n') {
            inputBuffer.push_back('\r');
        } else {
            inputBuffer.push_back(c);
        }
    }

    if (!inputBuffer.empty()) {
        hasInput.store(true, std::memory_order_relaxed);
    }
}

bool SerialIOBridge::HasPendingInput(const Hardware::ESP8266& esp8266) const noexcept {
    return hasInput.load(std::memory_order_relaxed) || (aciaEspBridge && esp8266.HasRxData());
}

void SerialIOBridge::ProcessSerialInput(Hardware::ACIA& acia, int targetIps, std::atomic<bool>& pendingInterruptAny, Hardware::ESP8266* esp8266) {
    const bool hasKeyInput = hasInput.load(std::memory_order_relaxed);
    const bool hasEspInput = aciaEspBridge && (esp8266 != nullptr) && esp8266->HasRxData();
    if (!hasKeyInput && !hasEspInput) {
        return;
    }

    if (baudDelay > 0 || !acia.CanReceive()) {
        return;
    }

    Hardware::Byte chr = 0;
    bool gotByte = false;
    if (hasKeyInput) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty()) {
            chr = static_cast<Hardware::Byte>(inputBuffer.front());
            inputBuffer.pop_front();
            gotByte = true;
        }
        if (inputBuffer.empty()) {
            hasInput.store(false, std::memory_order_relaxed);
        }
    } else if (hasEspInput) {
        if (esp8266->TryPopRxByte(chr)) {
            gotByte = true;
            esp8266->RecordTraffic(Hardware::TrafficDirection::RX, Hardware::TrafficChannel::ATResponse, -1, "ESP8266 -> ACIA", std::string(1, static_cast<char>(chr)));
        }
    }

    if (gotByte) {
        acia.ReceiveData(chr);
        if (acia.HasIRQ()) {
            pendingInterruptAny.store(true, std::memory_order_relaxed);
        }

        int baudRate = acia.GetBaudRate();
        if (baudRate <= 0) {
            baudRate = 19200;
        }
        int ips = targetIps;
        if (ips <= 0) {
            ips = 1000000;
        }
        int frameBits = acia.GetFrameBits();
        if (frameBits <= 0) {
            frameBits = 10;
        }
        baudDelay = static_cast<int>((static_cast<int64_t>(ips) * frameBits) / baudRate);
        if (baudDelay <= 0) {
            baudDelay = 1;
        }
    }
}

void SerialIOBridge::ForwardOutputIfBridged(char c, Hardware::ESP8266& esp8266) {
    if (!aciaEspBridge) {
        return;
    }
    esp8266.RecordTraffic(Hardware::TrafficDirection::TX, Hardware::TrafficChannel::ATCommand, -1,
                          "ACIA -> ESP8266", std::string(1, c));
    esp8266.Write(0x5004, static_cast<uint8_t>(c));
}

void SerialIOBridge::SerializeBuffer(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(bufferMutex);
    auto qSize = static_cast<uint32_t>(inputBuffer.size());
    Hardware::ISerializable::Serialize(out, qSize);
    for (char chr : inputBuffer) {
        out.write(&chr, 1);
    }
}

bool SerialIOBridge::DeserializeBuffer(std::istream& inStream) {
    uint32_t qSize = 0;
    Hardware::ISerializable::Deserialize(inStream, qSize);
    if (!inStream || qSize > 10ULL * 1024 * 1024) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.clear();
    for (size_t i = 0; i < qSize; ++i) {
        char chr = '\0';
        inStream.read(&chr, 1);
        inputBuffer.push_back(chr);
    }
    hasInput.store(!inputBuffer.empty(), std::memory_order_relaxed);
    return true;
}

} // namespace Core

