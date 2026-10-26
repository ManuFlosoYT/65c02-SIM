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

void SerialIOBridge::ProcessSerialInput(Hardware::ACIA& acia, int targetIps, std::atomic<bool>& pendingInterruptAny) {
    if (!hasInput.load(std::memory_order_relaxed)) {
        return;
    }

    if (baudDelay > 0 || !acia.CanReceive()) {
        return;
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    if (!inputBuffer.empty() && acia.CanReceive() && baudDelay <= 0) {
        char chr = inputBuffer.front();
        inputBuffer.pop_front();

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
        baudDelay = (ips * 10) / baudRate;
    }

    if (inputBuffer.empty()) {
        hasInput.store(false, std::memory_order_relaxed);
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

