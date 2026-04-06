#include "Hardware/Comm/ESP8266.h"

namespace Hardware {

ESP8266::ESP8266()
    : statusReg(0),
      cmdReg(0),
      ctrlReg(0)
#ifndef TARGET_WASM
      ,
      sslContext(asio::ssl::context::sslv23)
#endif
{
#ifndef TARGET_WASM
    sslContext.set_default_verify_paths();
#endif
    Reset();
}

ESP8266::~ESP8266() {
    if (pingThread.joinable()) {
        stopPing.store(true);
        pingThread.join();
    }
    DisconnectAll();
    StopServer();
}

void ESP8266::Reset() {
    if (pingThread.joinable()) {
        stopPing.store(true);
        pingThread.join();
    }
    statusReg = 0;
    cmdReg = 0;
    ctrlReg = 0;
    currentState = ATState::Idle;
    commandBuffer.clear();
    while (!rxQueue.empty()) {
        rxQueue.pop();
    }
    cipsendBuffer.clear();
    echoEnabled = true;
    muxEnabled = false;
    wifiConnected = false;
    connectedSSID = "";
    cwMode = CWMode::Station;
    DisconnectAll();
    StopServer();
}

Byte ESP8266::Read(Word address) {
    switch (address & 0x03) {
        case 0: {  // Data Register
            std::lock_guard<std::mutex> lock(rxMutex);
            if (rxQueue.empty()) {
                return 0;
            }
            Byte data = rxQueue.front();
            rxQueue.pop();
            if (rxQueue.empty()) {
                statusReg.fetch_and(0x7F);
            }
            return data;
        }
        case 1:  // Status Register
            return statusReg.load(std::memory_order_relaxed);
        case 2:  // Control Register
            return ctrlReg;
        default:
            return 0;
    }
}

void ESP8266::Write(Word address, Byte data) {
    switch (address & 0x03) {
        case 0:  // Command Register
            if (currentState == ATState::WaitingCIPSENDData) {
                cipsendBuffer += static_cast<char>(data);
                cipsendReceived++;
                if (cipsendReceived >= cipsendLength) {
                    SendDataOnLink(cipsendLinkId, cipsendBuffer);
                    EnqueueResponse("\r\nSEND OK\r\n");
                    currentState = ATState::Idle;
                    cipsendBuffer.clear();
                }
            } else {
                if (data == '\n') {
                    ProcessCommand();
                } else if (data != '\r') {
                    commandBuffer += static_cast<char>(data);
                }
            }
            break;
        case 2:  // Control Register
            ctrlReg = data;
            if ((data & 0x01) != 0) {  // Reset bit
                Reset();
            }
            break;
        default:
            break;
    }
}

std::string ESP8266::GetName() const { return "ESP8266"; }

void ESP8266::Clock() {}

bool ESP8266::SaveState(std::ostream& out) const {
    Byte sReg = statusReg.load();
    out.write(reinterpret_cast<const char*>(&sReg), sizeof(sReg));  // NOLINT
    out.write(reinterpret_cast<const char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    out.write(reinterpret_cast<const char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT

    size_t cmdLen = commandBuffer.length();
    out.write(reinterpret_cast<const char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    out.write(commandBuffer.c_str(), static_cast<std::streamsize>(cmdLen));

    out.write(reinterpret_cast<const char*>(&echoEnabled), sizeof(echoEnabled));      // NOLINT
    out.write(reinterpret_cast<const char*>(&muxEnabled), sizeof(muxEnabled));        // NOLINT
    out.write(reinterpret_cast<const char*>(&wifiConnected), sizeof(wifiConnected));  // NOLINT
    out.write(reinterpret_cast<const char*>(&cwMode), sizeof(cwMode));                // NOLINT

    size_t ssidLen = connectedSSID.length();
    out.write(reinterpret_cast<const char*>(&ssidLen), sizeof(ssidLen));  // NOLINT
    out.write(connectedSSID.c_str(), static_cast<std::streamsize>(ssidLen));

    return out.good();
}

bool ESP8266::LoadState(std::istream& inStream) {
    DisconnectAll();
    StopServer();

    Byte sReg = 0;
    inStream.read(reinterpret_cast<char*>(&sReg), sizeof(sReg));  // NOLINT
    statusReg.store(sReg);
    inStream.read(reinterpret_cast<char*>(&cmdReg), sizeof(cmdReg));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&ctrlReg), sizeof(ctrlReg));      // NOLINT

    size_t cmdLen = 0;
    inStream.read(reinterpret_cast<char*>(&cmdLen), sizeof(cmdLen));  // NOLINT
    commandBuffer.assign(cmdLen, '\0');
    inStream.read(commandBuffer.data(), static_cast<std::streamsize>(cmdLen));

    inStream.read(reinterpret_cast<char*>(&echoEnabled), sizeof(echoEnabled));      // NOLINT
    inStream.read(reinterpret_cast<char*>(&muxEnabled), sizeof(muxEnabled));        // NOLINT
    inStream.read(reinterpret_cast<char*>(&wifiConnected), sizeof(wifiConnected));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&cwMode), sizeof(cwMode));                // NOLINT

    size_t ssidLen = 0;
    inStream.read(reinterpret_cast<char*>(&ssidLen), sizeof(ssidLen));  // NOLINT
    connectedSSID.assign(ssidLen, '\0');
    inStream.read(connectedSSID.data(), static_cast<std::streamsize>(ssidLen));

    currentState = ATState::Idle;
    return inStream.good();
}

}  // namespace Hardware
