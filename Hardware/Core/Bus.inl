// Bus.inl — Inline implementations for Bus

inline void Hardware::Bus::SetProfilingEnabled(bool enabled) {
    profilingEnabled = enabled;
}
inline uint32_t* Hardware::Bus::GetProfilerCounts() { return profilerCounts; }
