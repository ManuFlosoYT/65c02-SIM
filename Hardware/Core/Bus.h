#pragma once
#include <functional>
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

using BusWriteHook = std::function<void(Word /* address */, Byte /* data */)>;
using BusReadHook =
    std::function<void(Word /* address */, Byte /* data_read */)>;

constexpr Word MAX_ADDR = 0xFFFF;
constexpr uint32_t BUS_SIZE = (uint32_t)MAX_ADDR + 1;
constexpr Word ROM_SIZE = 0x8000;

#define ACIA_DATA 0x5000
#define ACIA_STATUS 0x5001
#define ACIA_CMD 0x5002
#define ACIA_CTRL 0x5003

// VIA Registers
#define PORTB 0x6000
#define PORTA 0x6001
#define DDRB 0x6002
#define DDRA 0x6003
#define T1C_L 0x6004
#define T1C_H 0x6005
#define T1L_L 0x6006
#define T1L_H 0x6007
#define T2C_L 0x6008
#define T2C_H 0x6009
#define SR 0x600A
#define ACR 0x600B
#define PCR 0x600C
#define IFR 0x600D
#define IER 0x600E
#define ORA_NH 0x600F

struct DeviceRegistration {
    Word startAddress;
    Word endAddress;
    IBusDevice* device;
    bool enabled;
    bool ignoreCollision;
};

struct BusSlot {
    IBusDevice* device;
    Word offset;
};

class Bus {
public:
    Bus();
    void Init();
    void ClearDevices();

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    inline bool HasActiveHooks() const;

    void RegisterDevice(Word startAddress, Word endAddress, IBusDevice* device,
                        bool enabled = true, bool ignoreCollision = false);

    void UpdateDeviceRegistration(IBusDevice* device, Word newStart,
                                  Word newEnd, bool enabled,
                                  bool ignoreCollision);

    template <bool Debug>
    inline Byte Read(Word address);
    inline Byte Read(Word address);

    template <bool Debug>
    inline void Write(Word address, Byte data);
    inline void Write(Word address, Byte data);

    inline void WriteDirect(Word address, Byte data);
    inline Byte ReadDirect(Word address) const;

    void AddGlobalWriteHook(BusWriteHook hook);
    void AddGlobalReadHook(BusReadHook hook);

    inline void SetProfilingEnabled(bool enabled);
    void ClearProfiler();
    inline uint32_t* GetProfilerCounts();

    const std::vector<DeviceRegistration>& GetRegisteredDevices() const;
    void RebuildDeviceMap();

    inline Byte* GetPageReadPtr(Word page) const;

private:
    void UpdateCache();

    BusSlot deviceMap[BUS_SIZE];
    Byte* pageReadMap[256];
    Byte* pageWriteMap[256];

    std::vector<DeviceRegistration> registeredDevices;
    std::vector<BusWriteHook> globalWriteHooks;
    std::vector<BusReadHook> globalReadHooks;
    bool hasWriteHooks = false;
    bool hasReadHooks = false;

    bool profilingEnabled = false;
    uint32_t profilerCounts[BUS_SIZE]{0};
};

}  // namespace Hardware


namespace Hardware {

inline void Bus::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
}

inline uint32_t* Bus::GetProfilerCounts() { return profilerCounts; }

inline bool Bus::HasActiveHooks() const {
    return profilingEnabled || hasReadHooks || hasWriteHooks;
}

template <bool Debug>
inline Byte Bus::Read(Word address) {
    if constexpr (Debug) {
        if (profilingEnabled) profilerCounts[address]++;
    }
    Byte data = 0;

    if (Byte* memoryBase = pageReadMap[address >> 8]) {
        data = memoryBase[address];
    } else {
        const auto& slot = deviceMap[address];
        if (slot.device) {
            data = slot.device->Read(slot.offset);
        }
    }

    if constexpr (Debug) {
        if (hasReadHooks) {
            for (auto& hook : globalReadHooks) {
                hook(address, data);
            }
        }
    }
    return data;
}

inline Byte Bus::ReadDirect(Word address) const {
    if (Byte* memoryBase = pageReadMap[address >> 8]) {
        return memoryBase[address];
    }
    const auto& slot = deviceMap[address];
    if (slot.device) {
        return slot.device->Read(slot.offset);
    }
    return 0;
}

template <bool Debug>
inline void Bus::Write(Word address, Byte data) {
    if constexpr (Debug) {
        if (profilingEnabled) profilerCounts[address]++;
    }

    if (Byte* memoryBase = pageWriteMap[address >> 8]) {
        memoryBase[address] = data;
    } else {
        const auto& slot = deviceMap[address];
        if (slot.device) {
            slot.device->Write(slot.offset, data);
        }
    }

    if constexpr (Debug) {
        if (hasWriteHooks) {
            for (auto& hook : globalWriteHooks) {
                hook(address, data);
            }
        }
    }
}

inline void Bus::WriteDirect(Word address, Byte data) {
    if (Byte* memoryBase = pageWriteMap[address >> 8]) {
        memoryBase[address] = data;
    } else {
        const auto& slot = deviceMap[address];
        if (slot.device) {
            slot.device->Write(slot.offset, data);
        }
    }
}

inline Byte* Bus::GetPageReadPtr(Word page) const { return pageReadMap[page]; }

inline Byte Bus::Read(Word address) {
    if (HasActiveHooks()) {
        return Read<true>(address);
    } else {
        return Read<false>(address);
    }
}

inline void Bus::Write(Word address, Byte data) {
    if (HasActiveHooks()) {
        Write<true>(address, data);
    } else {
        Write<false>(address, data);
    }
}

}  // namespace Hardware
