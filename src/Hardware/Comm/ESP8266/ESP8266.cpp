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
    {
        std::lock_guard<std::mutex> lock(rxMutex);
        while (!rxQueue.empty()) {
            rxQueue.pop();
        }
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
    ISerializable::Serialize(out, sReg);
    ISerializable::Serialize(out, cmdReg);
    ISerializable::Serialize(out, ctrlReg);
    ISerializable::Serialize(out, commandBuffer);
    
    ISerializable::Serialize(out, echoEnabled);
    ISerializable::Serialize(out, muxEnabled);
    ISerializable::Serialize(out, wifiConnected);
    ISerializable::Serialize(out, cwMode);
    ISerializable::Serialize(out, connectedSSID);

    return out.good();
}

bool ESP8266::LoadState(std::istream& inStream) {
    DisconnectAll();
    StopServer();

    Byte sReg = 0;
    ISerializable::Deserialize(inStream, sReg);
    statusReg.store(sReg);
    ISerializable::Deserialize(inStream, cmdReg);
    ISerializable::Deserialize(inStream, ctrlReg);
    ISerializable::Deserialize(inStream, commandBuffer);

    ISerializable::Deserialize(inStream, echoEnabled);
    ISerializable::Deserialize(inStream, muxEnabled);
    ISerializable::Deserialize(inStream, wifiConnected);
    ISerializable::Deserialize(inStream, cwMode);
    ISerializable::Deserialize(inStream, connectedSSID);

    currentState = ATState::Idle;
    return inStream.good();
}

}  // namespace Hardware
