#include "Hardware/Core/Bus.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace Hardware {

Bus::Bus() { Init(); }

void Bus::Init() {
    for (int i = 0; i < BUS_SIZE; ++i) {
        deviceMap[i].device = nullptr;
        deviceMap[i].baseAddress = 0;
    }
    globalWriteHooks.clear();
    globalReadHooks.clear();
    RebuildDeviceMap();
}

void Bus::ClearDevices() {
    registeredDevices.clear();
    RebuildDeviceMap();
}

bool Bus::SaveState(std::ostream& out) const {
    uint32_t deviceCount = static_cast<uint32_t>(registeredDevices.size());
    out.write(reinterpret_cast<const char*>(&deviceCount), sizeof(deviceCount));

    for (const auto& reg : registeredDevices) {
        out.write(reinterpret_cast<const char*>(&reg.startAddress),
                  sizeof(reg.startAddress));
        out.write(reinterpret_cast<const char*>(&reg.endAddress),
                  sizeof(reg.endAddress));
        out.write(reinterpret_cast<const char*>(&reg.enabled),
                  sizeof(reg.enabled));
        out.write(reinterpret_cast<const char*>(&reg.ignoreCollision),
                  sizeof(reg.ignoreCollision));

        std::string name = reg.device->GetName();
        uint32_t nameLen = static_cast<uint32_t>(name.length());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        out.write(name.c_str(), nameLen);
    }
    return out.good();
}

bool Bus::LoadState(std::istream& in) {
    uint32_t deviceCount = 0;
    in.read(reinterpret_cast<char*>(&deviceCount), sizeof(deviceCount));

    for (uint32_t i = 0; i < deviceCount; ++i) {
        Word startAddress, endAddress;
        bool enabled, ignoreCollision;
        in.read(reinterpret_cast<char*>(&startAddress), sizeof(startAddress));
        in.read(reinterpret_cast<char*>(&endAddress), sizeof(endAddress));
        in.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
        in.read(reinterpret_cast<char*>(&ignoreCollision),
                sizeof(ignoreCollision));

        uint32_t nameLen = 0;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        in.read(&name[0], nameLen);

        if (i < registeredDevices.size()) {
            const auto& reg = registeredDevices[i];
            if (reg.device->GetName() != name ||
                reg.startAddress != startAddress ||
                reg.endAddress != endAddress) {
                std::cerr << "Memory map mismatch! Savestate: " << name
                          << " [0x" << std::hex << startAddress << "-0x"
                          << endAddress
                          << "], Current: " << reg.device->GetName() << " [0x"
                          << reg.startAddress << "-0x" << reg.endAddress << "]"
                          << std::dec << std::endl;
            }
        } else {
            std::cerr << "Memory map mismatch! Savestate has more devices ("
                      << deviceCount << ") than current configuration ("
                      << registeredDevices.size() << ")" << std::endl;
        }
    }

    if (deviceCount != registeredDevices.size()) {
        std::cerr << "Memory map count mismatch! Savestate: " << deviceCount
                  << ", Current: " << registeredDevices.size() << std::endl;
        return false;
    }

    return in.good();
}

void Bus::RegisterDevice(Word startAddress, Word endAddress, IBusDevice* device,
                         bool enabled, bool ignoreCollision) {
    if (enabled && !ignoreCollision) {
        for (int i = startAddress; i <= (int)endAddress; ++i) {
            if (deviceMap[i].device != nullptr) {
                throw std::runtime_error(
                    "Memory collision detected while registering device: " +
                    device->GetName() + " at address 0x" + std::to_string(i));
            }
        }
    }

    registeredDevices.push_back(
        {startAddress, endAddress, device, enabled, ignoreCollision});
    RebuildDeviceMap();
}

void Bus::UpdateDeviceRegistration(IBusDevice* device, Word newStart,
                                   Word newEnd, bool enabled,
                                   bool ignoreCollision) {
    for (auto& reg : registeredDevices) {
        if (reg.device == device) {
            reg.startAddress = newStart;
            reg.endAddress = newEnd;
            reg.enabled = enabled;
            reg.ignoreCollision = ignoreCollision;
            break;
        }
    }
    RebuildDeviceMap();
}

void Bus::Write(Word address, Byte data) {
    if (profilingEnabled) profilerCounts[address]++;

    if (deviceMap[address].device != nullptr) {
        deviceMap[address].device->Write(
            address - deviceMap[address].baseAddress, data);
    }

    for (auto& hook : globalWriteHooks) {
        hook(address, data);
    }
}

void Bus::WriteDirect(Word address, Byte data) {
    if (deviceMap[address].device != nullptr) {
        deviceMap[address].device->Write(
            address - deviceMap[address].baseAddress, data);
    }
}

void Bus::AddGlobalWriteHook(BusWriteHook hook) {
    globalWriteHooks.push_back(hook);
}

void Bus::AddGlobalReadHook(BusReadHook hook) {
    globalReadHooks.push_back(hook);
}

void Bus::ClearProfiler() {
    std::memset(profilerCounts, 0, sizeof(profilerCounts));
}

const std::vector<DeviceRegistration>& Bus::GetRegisteredDevices() const {
    return registeredDevices;
}

void Bus::RebuildDeviceMap() {
    for (int i = 0; i < BUS_SIZE; ++i) {
        deviceMap[i].device = nullptr;
        deviceMap[i].baseAddress = 0;
    }

    for (const auto& reg : registeredDevices) {
        if (reg.enabled && !reg.ignoreCollision) {
            for (int i = reg.startAddress; i <= (int)reg.endAddress; ++i) {
                if (deviceMap[i].device != nullptr) {
                    throw std::runtime_error(
                        "Memory collision detected during "
                        "RebuildDeviceMap for: " +
                        reg.device->GetName());
                }
                deviceMap[i].device = reg.device;
                deviceMap[i].baseAddress = reg.startAddress;
            }
        }
    }

    for (const auto& reg : registeredDevices) {
        if (reg.enabled && reg.ignoreCollision) {
            for (int i = reg.startAddress; i <= (int)reg.endAddress; ++i) {
                deviceMap[i].device = reg.device;
                deviceMap[i].baseAddress = reg.startAddress;
            }
        }
    }

    UpdateCache();
}

void Bus::UpdateCache() {
    for (int i = 0; i < BUS_SIZE; ++i) {
        if (deviceMap[i].device != nullptr) {
            // Unbind the device and its correct address offset into a fast
            // lambda
            auto* dev = deviceMap[i].device;
            Word offset = i - deviceMap[i].baseAddress;
            readCache[i] = [dev, offset]() -> Byte {
                return dev->Read(offset);
            };
        } else {
            readCache[i] = nullptr;
        }
    }
}

}  // namespace Hardware
