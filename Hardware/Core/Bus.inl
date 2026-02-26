// Bus.inl — Inline implementations for Bus

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
