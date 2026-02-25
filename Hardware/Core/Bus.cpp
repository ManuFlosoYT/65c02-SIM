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
    return out.good();
}

bool Bus::LoadState(std::istream& in) {
    return in.good();
}

void Bus::RegisterDevice(Word startAddress, Word endAddress, IBusDevice* device,
                         bool enabled, bool ignoreCollision) {
    if (enabled && !ignoreCollision) {
        for (int i = startAddress; i <= (int)endAddress; ++i) {
            if (deviceMap[i].device != nullptr) {
                throw std::runtime_error(
                    "Memory collision detected while registering device: " +
                    device->GetName() + " at address 0x" +
                    std::to_string(i));
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

Byte Bus::Read(Word address) {
    if (profilingEnabled) profilerCounts[address]++;
    Byte data = 0;

    if (deviceMap[address].device != nullptr) {
        data = deviceMap[address].device->Read(address -
                                               deviceMap[address].baseAddress);
    }

    for (auto& hook : globalReadHooks) {
        hook(address, data);
    }
    return data;
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

Byte Bus::ReadDirect(Word address) const {
    if (deviceMap[address].device != nullptr) {
        return deviceMap[address].device->Read(address -
                                               deviceMap[address].baseAddress);
    }
    return 0;
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
}

}  // namespace Hardware
