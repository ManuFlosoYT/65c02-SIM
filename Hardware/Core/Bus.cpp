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
        deviceMap[i].offset = 0;
    }
    for (int i = 0; i < 256; ++i) {
        pageReadMap[i] = nullptr;
        pageWriteMap[i] = nullptr;
    }
    hasWriteHooks = false;
    hasReadHooks = false;
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

void Bus::AddGlobalWriteHook(BusWriteHook hook) {
    globalWriteHooks.push_back(hook);
    hasWriteHooks = !globalWriteHooks.empty();
}

void Bus::AddGlobalReadHook(BusReadHook hook) {
    globalReadHooks.push_back(hook);
    hasReadHooks = !globalReadHooks.empty();
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
        deviceMap[i].offset = 0;
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
                deviceMap[i].offset = i - reg.startAddress;
            }
        }
    }

    for (const auto& reg : registeredDevices) {
        if (reg.enabled && reg.ignoreCollision) {
            for (int i = reg.startAddress; i <= (int)reg.endAddress; ++i) {
                deviceMap[i].device = reg.device;
                deviceMap[i].offset = i - reg.startAddress;
            }
        }
    }

    UpdateCache();
}

void Bus::UpdateCache() {
    for (int p = 0; p < 256; ++p) {
        pageReadMap[p] = nullptr;
        pageWriteMap[p] = nullptr;

        bool pageIsUniform = true;
        IBusDevice* firstDevice = deviceMap[p << 8].device;
        if (!firstDevice || !firstDevice->GetRawMemory()) {
            pageIsUniform = false;
        } else {
            Word expectedOffset = deviceMap[p << 8].offset;
            for (int i = 0; i < 256; ++i) {
                Word addr = (p << 8) | i;
                if (deviceMap[addr].device != firstDevice) {
                    pageIsUniform = false;
                    break;
                }
                if (deviceMap[addr].offset != expectedOffset + i) {
                    pageIsUniform = false;
                    break;
                }
            }
        }

        if (pageIsUniform) {
            Byte* rawPtr = firstDevice->GetRawMemory();
            Byte* memoryBase = rawPtr + deviceMap[p << 8].offset - (p << 8);

            pageReadMap[p] = memoryBase;
            if (!firstDevice->IsReadOnly()) {
                pageWriteMap[p] = memoryBase;
            }
        }
    }
}

}  // namespace Hardware
