#pragma once
#include <array>
#include <functional>
#include <vector>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

using BusWriteHook = std::function<void(Word /* address */, Byte /* data */)>;
using BusReadHook = std::function<void(Word /* address */, Byte /* data_read */)>;

constexpr Word MAX_ADDR = 0xFFFF;
constexpr uint32_t BUS_SIZE = (uint32_t)MAX_ADDR + 1;
constexpr Word ROM_SIZE = 0x8000;

constexpr Word ACIA_DATA = 0x5000;
constexpr Word ACIA_STATUS = 0x5001;
constexpr Word ACIA_CMD = 0x5002;
constexpr Word ACIA_CTRL = 0x5003;

// VIA Registers
constexpr Word PORTB = 0x6000;
constexpr Word PORTA = 0x6001;
constexpr Word DDRB = 0x6002;
constexpr Word DDRA = 0x6003;
constexpr Word T1C_L = 0x6004;
constexpr Word T1C_H = 0x6005;
constexpr Word T1L_L = 0x6006;
constexpr Word T1L_H = 0x6007;
constexpr Word T2C_L = 0x6008;
constexpr Word T2C_H = 0x6009;
constexpr Word SR = 0x600A;  // NOLINT
constexpr Word ACR = 0x600B;
constexpr Word PCR = 0x600C;
constexpr Word IFR = 0x600D;
constexpr Word IER = 0x600E;
constexpr Word ORA_NH = 0x600F;

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
    bool LoadState(std::istream& inStream);

    [[nodiscard]] inline bool HasActiveHooks() const;

    void RegisterDevice(Word startAddress, Word endAddress, IBusDevice* device, bool enabled = true,
                        bool ignoreCollision = false);

    void UpdateDeviceRegistration(IBusDevice* device, Word newStart, Word newEnd, bool enabled, bool ignoreCollision);

    template <bool Debug>
    inline Byte Read(Word address);
    inline Byte Read(Word address);

    template <bool Debug>
    inline void Write(Word address, Byte data);
    inline void Write(Word address, Byte data);

    inline void WriteDirect(Word address, Byte data);
    [[nodiscard]] inline Byte ReadDirect(Word address) const;

    void AddGlobalWriteHook(const BusWriteHook& hook);
    void AddGlobalReadHook(const BusReadHook& hook);

    inline void SetProfilingEnabled(bool enabled);
    void ClearProfiler();
    inline uint32_t* GetProfilerCounts();

    [[nodiscard]] const std::vector<DeviceRegistration>& GetRegisteredDevices() const;
    void RebuildDeviceMap();

    [[nodiscard]] inline Byte* GetPageReadPtr(Word page) const;

   private:
    void UpdateCache();

    std::array<BusSlot, BUS_SIZE> deviceMap{};
    std::array<Byte*, 256> pageReadMap{};
    std::array<Byte*, 256> pageWriteMap{};

    std::vector<DeviceRegistration> registeredDevices;
    std::vector<BusWriteHook> globalWriteHooks;
    std::vector<BusReadHook> globalReadHooks;
    bool hasWriteHooks = false;
    bool hasReadHooks = false;

    bool profilingEnabled = false;
    std::array<uint32_t, BUS_SIZE> profilerCounts{};
};

}  // namespace Hardware

namespace Hardware {

inline void Bus::SetProfilingEnabled(bool enabled) { profilingEnabled = enabled; }

inline uint32_t* Bus::GetProfilerCounts() { return profilerCounts.data(); }

inline bool Bus::HasActiveHooks() const { return profilingEnabled || hasReadHooks || hasWriteHooks; }

template <bool Debug>
inline Byte Bus::Read(Word address) {
    if constexpr (Debug) {
        if (profilingEnabled) {
            profilerCounts[address]++;  // NOLINT
        }
    }
    Byte data = 0;

    if (Byte* memoryBase = pageReadMap[address >> 8]) {  // NOLINT
        data = memoryBase[address];                      // NOLINT
    } else {
        const auto& slot = deviceMap[address];  // NOLINT
        if (slot.device != nullptr) {
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
    if (Byte* memoryBase = pageReadMap[address >> 8]) {  // NOLINT
        return memoryBase[address];                      // NOLINT
    }
    const auto& slot = deviceMap[address];  // NOLINT
    if (slot.device != nullptr) {
        return slot.device->Read(slot.offset);
    }
    return 0;
}

template <bool Debug>
inline void Bus::Write(Word address, Byte data) {
    if constexpr (Debug) {
        if (profilingEnabled) {
            profilerCounts[address]++;  // NOLINT
        }
    }

    if (Byte* memoryBase = pageWriteMap[address >> 8]) {  // NOLINT
        memoryBase[address] = data;                       // NOLINT
    } else {
        const auto& slot = deviceMap[address];  // NOLINT
        if (slot.device != nullptr) {
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
    if (Byte* memoryBase = pageWriteMap[address >> 8]) {  // NOLINT
        memoryBase[address] = data;                       // NOLINT
    } else {
        const auto& slot = deviceMap[address];  // NOLINT
        if (slot.device != nullptr) {
            slot.device->Write(slot.offset, data);
        }
    }
}

inline Byte* Bus::GetPageReadPtr(Word page) const { return pageReadMap[page]; }  // NOLINT

inline Byte Bus::Read(Word address) {
    if (HasActiveHooks()) {
        return Read<true>(address);
    }
    return Read<false>(address);
}

inline void Bus::Write(Word address, Byte data) {
    if (HasActiveHooks()) {
        Write<true>(address, data);
    } else {
        Write<false>(address, data);
    }
}

}  // namespace Hardware
