// Bus.inl — Inline implementations for Bus

namespace Hardware {

inline void Bus::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
}

inline uint32_t* Bus::GetProfilerCounts() { return profilerCounts; }

inline Byte Bus::Read(Word address) {
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

inline Byte Bus::ReadDirect(Word address) const {
    if (readCache[address]) {
        return readCache[address]();
    }
    return 0;
}

}  // namespace Hardware
