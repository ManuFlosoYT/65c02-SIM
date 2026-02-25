// Bus.inl — Inline implementations for Bus

namespace Hardware {

inline void Bus::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
}

inline uint32_t* Bus::GetProfilerCounts() { return profilerCounts; }

inline Byte Bus::Read(Word address) {
    if (profilingEnabled) profilerCounts[address]++;
    Byte data = 0;

    const auto& slot = fastCache[address];
    if (slot.rawPtr) {
        data = *slot.rawPtr;
    } else if (slot.device) {
        data = slot.device->Read(slot.offset);
    }

    for (auto& hook : globalReadHooks) {
        hook(address, data);
    }
    return data;
}

inline Byte Bus::ReadDirect(Word address) const {
    const auto& slot = fastCache[address];
    if (slot.rawPtr) {
        return *slot.rawPtr;
    }
    if (slot.device) {
        return slot.device->Read(slot.offset);
    }
    return 0;
}

}  // namespace Hardware
