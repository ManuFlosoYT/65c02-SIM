#include "Hardware/Core/Bus.h"

#include <format>
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
    ISerializable::Serialize(out, deviceCount);

    for (const auto& reg : registeredDevices) {
        ISerializable::Serialize(out, reg.startAddress);
        ISerializable::Serialize(out, reg.endAddress);
        ISerializable::Serialize(out, reg.enabled);
        ISerializable::Serialize(out, reg.ignoreCollision);

        ISerializable::Serialize(out, reg.device->GetName());
    }
    return out.good();
}

bool Bus::LoadState(std::istream& inStream) {
    uint32_t deviceCount = 0;
    ISerializable::Deserialize(inStream, deviceCount);

    for (uint32_t i = 0; i < deviceCount; ++i) {
        Word startAddress = 0;
        Word endAddress = 0;
        bool enabled = false;
        bool ignoreCollision = false;
        ISerializable::Deserialize(inStream, startAddress);
        ISerializable::Deserialize(inStream, endAddress);
        ISerializable::Deserialize(inStream, enabled);
        ISerializable::Deserialize(inStream, ignoreCollision);

        std::string name;
        ISerializable::Deserialize(inStream, name);

        if (i < registeredDevices.size()) {
            const auto& reg = registeredDevices.at(i);
            if (reg.device->GetName() != name || reg.startAddress != startAddress || reg.endAddress != endAddress) {
                std::cerr << "Memory map mismatch! Savestate: " << name << " [0x" << std::hex << startAddress << "-0x"
                          << endAddress << "], Current: " << reg.device->GetName() << " [0x" << reg.startAddress
                          << "-0x" << reg.endAddress << "]" << std::dec << '\n';
                return false;
            }
        } else {
            std::cerr << "Memory map mismatch! Savestate has more devices (" << deviceCount
                      << ") than current configuration (" << registeredDevices.size() << ")" << '\n';
            return false;
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
        for (int i = static_cast<int>(startAddress); i <= static_cast<int>(endAddress); ++i) {
            if (deviceMap.at(i).device != nullptr) {
                throw std::runtime_error(std::format("Memory collision detected while registering device: {} at address 0x{:04X}",
                                                     device->GetName(), i));
            }
        }
    }

    registeredDevices.push_back({startAddress, endAddress, device, enabled, ignoreCollision, false});
    RebuildDeviceMap();
}

void Bus::RegisterVirtualDevice(IBusDevice* device, bool enabled) {
    // Virtual devices are UI-only: no bus address, no collision check, never mapped.
    registeredDevices.push_back({0, 0, device, enabled, false, true});
    // No RebuildDeviceMap needed — deviceMap is unaffected.
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
std::vector<DeviceRegistration>& Bus::GetRegisteredDevices() { return registeredDevices; }

void Bus::RebuildDeviceMap() {
    deviceMap.fill({nullptr, 0});

    for (const auto& reg : registeredDevices) {
        if (reg.isVirtual) { continue; }
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
        if (reg.isVirtual) { continue; }
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
            size_t devSize = firstDevice->GetRawMemorySize();
            auto baseOffset = static_cast<size_t>(deviceMap.at(page << 8).offset);

            if (baseOffset >= devSize || (devSize - baseOffset) < 256) {
                throw std::runtime_error(std::format(
                    "Invalid memory mapping for device {}: page 0x{:02X} exceeds raw memory size (offset=0x{:04X}, size=0x{:04X})",
                    firstDevice->GetName(), page, baseOffset, devSize));
            }

            std::span<Byte> rawSpan(rawPtr, devSize);
            Byte* memoryBase = &rawSpan[baseOffset];

            pageReadMap.at(page) = memoryBase;

            if (!firstDevice->IsReadOnly()) {
                pageWriteMap.at(page) = memoryBase;
            }
        }
    }
}

}  // namespace Hardware
