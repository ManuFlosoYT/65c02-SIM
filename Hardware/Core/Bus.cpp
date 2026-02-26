#include "Hardware/Core/Bus.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace Hardware {

Bus::Bus() { Init(); }

void Bus::Init() {
    deviceMap.fill({nullptr, 0});
    pageReadMap.fill(nullptr);
    pageWriteMap.fill(nullptr);
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
    auto deviceCount = static_cast<uint32_t>(registeredDevices.size());
    out.write(reinterpret_cast<const char*>(&deviceCount), sizeof(deviceCount));  // NOLINT

    for (const auto& reg : registeredDevices) {
        out.write(reinterpret_cast<const char*>(&reg.startAddress),  // NOLINT
                  sizeof(reg.startAddress));
        out.write(reinterpret_cast<const char*>(&reg.endAddress),  // NOLINT
                  sizeof(reg.endAddress));
        out.write(reinterpret_cast<const char*>(&reg.enabled),  // NOLINT
                  sizeof(reg.enabled));
        out.write(reinterpret_cast<const char*>(&reg.ignoreCollision),  // NOLINT
                  sizeof(reg.ignoreCollision));

        std::string name = reg.device->GetName();
        auto nameLen = static_cast<uint32_t>(name.length());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));  // NOLINT
        out.write(name.c_str(), nameLen);
    }
    return out.good();
}

bool Bus::LoadState(std::istream& inStream) {
    uint32_t deviceCount = 0;
    inStream.read(reinterpret_cast<char*>(&deviceCount), sizeof(deviceCount));  // NOLINT

    for (uint32_t i = 0; i < deviceCount; ++i) {
        Word startAddress = 0;
        Word endAddress = 0;
        bool enabled = false;
        bool ignoreCollision = false;
        inStream.read(reinterpret_cast<char*>(&startAddress), sizeof(startAddress));  // NOLINT
        inStream.read(reinterpret_cast<char*>(&endAddress), sizeof(endAddress));      // NOLINT
        inStream.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));            // NOLINT
        inStream.read(reinterpret_cast<char*>(&ignoreCollision),                      // NOLINT
                      sizeof(ignoreCollision));

        uint32_t nameLen = 0;
        inStream.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));  // NOLINT
        std::string name(nameLen, '\0');
        inStream.read(name.data(), nameLen);

        if (i < registeredDevices.size()) {
            const auto& reg = registeredDevices.at(i);
            if (reg.device->GetName() != name || reg.startAddress != startAddress || reg.endAddress != endAddress) {
                std::cerr << "Memory map mismatch! Savestate: " << name << " [0x" << std::hex << startAddress << "-0x"
                          << endAddress << "], Current: " << reg.device->GetName() << " [0x" << reg.startAddress
                          << "-0x" << reg.endAddress << "]" << std::dec << '\n';
            }
        } else {
            std::cerr << "Memory map mismatch! Savestate has more devices (" << deviceCount
                      << ") than current configuration (" << registeredDevices.size() << ")" << '\n';
        }
    }

    if (deviceCount != registeredDevices.size()) {
        std::cerr << "Memory map count mismatch! Savestate: " << deviceCount
                  << ", Current: " << registeredDevices.size() << '\n';
        return false;
    }

    return inStream.good();
}

void Bus::RegisterDevice(Word startAddress, Word endAddress, IBusDevice* device, bool enabled, bool ignoreCollision) {
    if (enabled && !ignoreCollision) {
        for (int i = startAddress; i <= (int)endAddress; ++i) {
            if (deviceMap.at(i).device != nullptr) {
                throw std::runtime_error("Memory collision detected while registering device: " + device->GetName() +
                                         " at address 0x" + std::to_string(i));
            }
        }
    }

    registeredDevices.push_back({startAddress, endAddress, device, enabled, ignoreCollision});
    RebuildDeviceMap();
}

void Bus::UpdateDeviceRegistration(IBusDevice* device, Word newStart, Word newEnd, bool enabled, bool ignoreCollision) {
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

void Bus::AddGlobalWriteHook(const BusWriteHook& hook) {
    globalWriteHooks.push_back(hook);
    hasWriteHooks = !globalWriteHooks.empty();
}

void Bus::AddGlobalReadHook(const BusReadHook& hook) {
    globalReadHooks.push_back(hook);
    hasReadHooks = !globalReadHooks.empty();
}

void Bus::ClearProfiler() { profilerCounts.fill(0); }

const std::vector<DeviceRegistration>& Bus::GetRegisteredDevices() const { return registeredDevices; }

void Bus::RebuildDeviceMap() {
    deviceMap.fill({nullptr, 0});

    for (const auto& reg : registeredDevices) {
        if (reg.enabled && !reg.ignoreCollision) {
            for (int i = reg.startAddress; i <= (int)reg.endAddress; ++i) {
                if (deviceMap.at(i).device != nullptr) {
                    throw std::runtime_error(
                        "Memory collision detected during "
                        "RebuildDeviceMap for: " +
                        reg.device->GetName());
                }
                deviceMap.at(i).device = reg.device;
                deviceMap.at(i).offset = i - reg.startAddress;
            }
        }
    }

    for (const auto& reg : registeredDevices) {
        if (reg.enabled && reg.ignoreCollision) {
            for (int i = reg.startAddress; i <= (int)reg.endAddress; ++i) {
                deviceMap.at(i).device = reg.device;
                deviceMap.at(i).offset = i - reg.startAddress;
            }
        }
    }

    UpdateCache();
}

void Bus::UpdateCache() {
    for (int page = 0; page < 256; ++page) {
        pageReadMap.at(page) = nullptr;
        pageWriteMap.at(page) = nullptr;

        bool pageIsUniform = true;
        IBusDevice* firstDevice = deviceMap.at(page << 8).device;
        if (firstDevice == nullptr || firstDevice->GetRawMemory() == nullptr) {
            pageIsUniform = false;
        } else {
            Word expectedOffset = deviceMap.at(page << 8).offset;
            for (int i = 0; i < 256; ++i) {
                Word addr = (page << 8) | i;
                if (deviceMap.at(addr).device != firstDevice) {
                    pageIsUniform = false;
                    break;
                }
                if (deviceMap.at(addr).offset != expectedOffset + i) {
                    pageIsUniform = false;
                    break;
                }
            }
        }

        if (pageIsUniform) {
            Byte* rawPtr = firstDevice->GetRawMemory();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            Byte* memoryBase = rawPtr + deviceMap.at(page << 8).offset - (page << 8);

            pageReadMap.at(page) = memoryBase;
            if (!firstDevice->IsReadOnly()) {
                pageWriteMap.at(page) = memoryBase;
            }
        }
    }
}

}  // namespace Hardware
