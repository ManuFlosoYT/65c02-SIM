// Bus.inl — Inline implementations for Bus

namespace Hardware {

inline void Bus::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
}

inline uint32_t* Bus::GetProfilerCounts() { return profilerCounts; }

inline Byte Bus::Read(Word address) {
    if (profilingEnabled) profilerCounts[address]++;
    Byte data = 0;

    if (Byte* memoryBase = pageReadMap[address >> 8]) {
        data = memoryBase[address];
    } else {
        const auto& slot = deviceMap[address];
        if (slot.device) {
            data = slot.device->Read(slot.offset);
        }
    }

    if (hasReadHooks) {
        for (auto& hook : globalReadHooks) {
            hook(address, data);
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

inline void Bus::Write(Word address, Byte data) {
    if (profilingEnabled) profilerCounts[address]++;

    if (Byte* memoryBase = pageWriteMap[address >> 8]) {
        memoryBase[address] = data;
    } else {
        const auto& slot = deviceMap[address];
        if (slot.device) {
            slot.device->Write(slot.offset, data);
        }
    }

    if (hasWriteHooks) {
        for (auto& hook : globalWriteHooks) {
            hook(address, data);
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

}  // namespace Hardware
