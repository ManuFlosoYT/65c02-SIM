// Emulator.inl — Inline implementations for Core::Emulator

inline void Core::Emulator::SetLCDOutputCallback(std::function<void(char)> cb) {
    lcd.SetOutputCallback(cb);
}

inline const char (&Core::Emulator::GetLCDScreen() const)[2][16] {
    return lcd.GetScreen();
}
inline const Hardware::LCD& Core::Emulator::GetLCD() const { return lcd; }
inline const Hardware::CPU& Core::Emulator::GetCPU() const { return cpu; }
inline Hardware::CPU& Core::Emulator::GetCPU() { return cpu; }
inline const Hardware::Bus& Core::Emulator::GetMem() const { return bus; }
inline Hardware::Bus& Core::Emulator::GetMem() { return bus; }
inline Hardware::GPU& Core::Emulator::GetGPU() { return gpu; }
inline void Core::Emulator::SetGPUEnabled(bool enabled) {
    gpuEnabled = enabled;
}
inline bool Core::Emulator::IsGPUEnabled() const { return gpuEnabled; }

inline Hardware::SID& Core::Emulator::GetSID() { return sid; }
inline Hardware::VIA& Core::Emulator::GetVIA() { return via; }

inline void Core::Emulator::SetCycleAccurate(bool enabled) {
    cpu.SetCycleAccurate(enabled);
}
inline bool Core::Emulator::IsCycleAccurate() const {
    return cpu.IsCycleAccurate();
}

inline bool Core::Emulator::IsRunning() const { return running; }
inline bool Core::Emulator::IsPaused() const { return paused; }

inline void Core::Emulator::SetTargetIPS(int ips) { targetIPS = ips; }
inline int Core::Emulator::GetTargetIPS() const { return targetIPS; }
inline int Core::Emulator::GetActualIPS() const { return actualIPS; }

inline void Core::Emulator::SetProfilingEnabled(bool enabled) {
    bus.SetProfilingEnabled(enabled);
}
inline void Core::Emulator::ClearProfiler() { bus.ClearProfiler(); }
inline uint32_t* Core::Emulator::GetProfilerCounts() {
    return bus.GetProfilerCounts();
}

inline void Core::Emulator::SetAutoReload(bool enabled) {
    autoReloadRequested = enabled;
}
inline bool Core::Emulator::IsAutoReloadEnabled() const {
    return autoReloadRequested;
}

inline Core::SavestateLoadResult Core::Emulator::GetLastLoadResult() const {
    return lastLoadResult;
}
inline std::string Core::Emulator::GetLastLoadVersion() const {
    return lastLoadVersion;
}
inline std::string Core::Emulator::GetCurrentBinPath() const {
    return currentBinPath;
}
