#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "Hardware/Core/ISerializable.h"

namespace Hardware {

enum class CWMode : uint8_t {
    Station = 1,
    SoftAP = 2,
    StationAndAP = 3
};

struct VirtualWifiState {
    CWMode cwMode{CWMode::Station};
    bool wifiConnected{false};
    std::string connectedSSID;

    void Reset() {
        cwMode = CWMode::Station;
        wifiConnected = false;
        connectedSSID.clear();
    }

    bool SaveState(std::ostream& out) const {
        ISerializable::Serialize(out, wifiConnected);
        ISerializable::Serialize(out, cwMode);
        ISerializable::Serialize(out, connectedSSID);
        return out.good();
    }

    bool LoadState(std::istream& inStream) {
        ISerializable::Deserialize(inStream, wifiConnected);
        ISerializable::Deserialize(inStream, cwMode);
        ISerializable::Deserialize(inStream, connectedSSID);
        return inStream.good();
    }
};

}  // namespace Hardware
