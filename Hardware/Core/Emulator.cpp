#include "Hardware/Core/Emulator.h"

#include <picosha2.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "Frontend/Control/Console.h"

using namespace Hardware;

namespace Core {

Emulator::Emulator() : bus(), cpu(), lcd(), ram(), rom(), acia(), via() {}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(emulationMutex);
    bus.Init();
    cpu.Reset();

    if (bus.GetRegisteredDevices().empty()) {
        bus.RegisterDevice(0x0000, 0x7FFF, &ram, true, false);
        bus.RegisterDevice(0x8000, 0xFFFF, &rom, true, false);
        bus.RegisterDevice(0x5000, 0x5003, &acia, true, true);
        bus.RegisterDevice(0x6000, 0x600F, &via, true, true);
        bus.RegisterDevice(0x4800, 0x481F, &sid, true, true);
        bus.RegisterDevice(0x2000, 0x3FFF, &gpu, true, true);
    }

    acia.Reset();
    via.Reset();
    sid.Reset();
    gpu.Reset();
    lcd.Reset();

    via.SetPortBCallback([this](Byte val) { lcd.Update(val); });

    std::string path = bin;

    FILE* file = fopen(path.c_str(), "rb");

    if (file == nullptr && path.find(".bin") == std::string::npos) {
        std::string tryBin = path + ".bin";
        file = fopen(tryBin.c_str(), "rb");
        if (file != nullptr) {
            path = tryBin;
        }
    }
    if (file == nullptr) {
        errorMsg = "Error opening file " + path;
        return false;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize != ROM_SIZE) {
        errorMsg = "Error: The file " + path + " does not have size " +
                   std::to_string(ROM_SIZE);
        fclose(file);
        return false;
    }
    fseek(file, 0, SEEK_SET);

    std::vector<Byte> buffer(ROM_SIZE);
    size_t bytesRead = fread(buffer.data(), 1, ROM_SIZE, file);
    if (bytesRead == 0) {
        errorMsg = "Error reading file " + path;
        fclose(file);
        return false;
    }

    for (Word i = 0; i < bytesRead; ++i) {
        rom.WriteDirect(i, buffer[i]);
    }

    fclose(file);

    currentBinPath = path;
    try {
        if (std::filesystem::exists(path)) {
            lastBinModificationTime = std::filesystem::last_write_time(path);
        }
    } catch (...) {
        // Ignore filesystem errors
    }

    return true;
}

bool Emulator::SaveState(const std::string& filename) {
    std::lock_guard<std::mutex> lock(emulationMutex);

    std::stringstream ss;

    if (!bus.SaveState(ss)) return false;

    std::string version = PROJECT_VERSION;
    uint32_t versionLen = static_cast<uint32_t>(version.length());
    ss.write(reinterpret_cast<const char*>(&versionLen), sizeof(versionLen));
    ss.write(version.c_str(), versionLen);

    if (!cpu.SaveState(ss)) return false;
    if (!ram.SaveState(ss)) return false;
    if (!rom.SaveState(ss)) return false;
    if (!via.SaveState(ss)) return false;
    if (!sid.SaveState(ss)) return false;
    if (!acia.SaveState(ss)) return false;
    if (!lcd.SaveState(ss)) return false;
    if (!gpu.SaveState(ss)) return false;
    if (!Console::SaveState(ss)) return false;

    std::string payload = ss.str();

    std::string hashHex = picosha2::hash256_hex_string(payload);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;

    const char magic[] = "SIM65C02SST";
    out.write(magic, sizeof(magic) - 1);

    out.write(payload.c_str(), payload.size());
    out.write(hashHex.c_str(), hashHex.size());

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool forceLoad) {
    std::lock_guard<std::mutex> lock(emulationMutex);

    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    const char magic[] = "SIM65C02SST";
    size_t magicLen = sizeof(magic) - 1;
    size_t hashLen = 64;

    if (size < (std::streamsize)(magicLen + hashLen)) return false;

    char fileMagic[11];
    in.read(fileMagic, magicLen);
    if (strncmp(fileMagic, magic, magicLen) != 0) {
        return false;
    }

    size_t payloadLen = size - magicLen - hashLen;
    std::string payload(payloadLen, '\0');
    in.read(&payload[0], payloadLen);

    char fileHash[64];
    in.read(fileHash, hashLen);

    if (!forceLoad) {
        std::string computedHash = picosha2::hash256_hex_string(payload);
        if (strncmp(fileHash, computedHash.c_str(), hashLen) != 0) {
            std::cerr << "Save state hash mismatch! Computed: " << computedHash
                      << " Expected: " << std::string(fileHash, hashLen)
                      << std::endl;
            return false;
        }
    }

    std::stringstream ss(payload);

    if (!bus.LoadState(ss) && !forceLoad) return false;

    uint32_t versionLen = 0;
    ss.read(reinterpret_cast<char*>(&versionLen), sizeof(versionLen));
    std::string version(versionLen, '\0');
    ss.read(&version[0], versionLen);

    bool versionMismatch = (version != PROJECT_VERSION);
    if (versionMismatch) {
        std::cerr << "Warning: Savestate version mismatch! File: " << version
                  << " Current: " << PROJECT_VERSION << std::endl;
    }

    bool success = true;
    if (!cpu.LoadState(ss)) success = false;
    if (success && !ram.LoadState(ss)) success = false;
    if (success && !rom.LoadState(ss)) success = false;
    if (success && !via.LoadState(ss)) success = false;
    if (success && !sid.LoadState(ss)) success = false;
    if (success && !acia.LoadState(ss)) success = false;
    if (success && !lcd.LoadState(ss)) success = false;
    if (success && !gpu.LoadState(ss)) success = false;
    if (success && !Console::LoadState(ss)) success = false;

    if (!success || (!ss.good() && !ss.eof())) {
        if (!forceLoad) {
            if (versionMismatch) {
                std::cerr << "Error: Savestate version " << version
                          << " is not compatible." << std::endl;
            }
            return false;
        }
    }

    for (Word addr = 0x2000; addr < 0x4000; addr++) {
        gpu.Write(addr - 0x2000, bus.ReadDirect(addr));
    }

    return forceLoad || (success && (ss.good() || ss.eof()));
}

int Emulator::Step() {
    int res = 0;

    if (gpuEnabled) {
        gpu.Clock();
        if (gpu.IsInBlankingInterval()) {
            res = cpu.Step(bus);
        }
    } else {
        res = cpu.Step(bus);
    }

    if (baudDelay > 0) baudDelay--;

    if ((bus.ReadDirect(ACIA_STATUS) & 0x80) != 0) {
        cpu.IRQ(bus);
    }

    via.Clock();
    if (via.isIRQAsserted()) {
        cpu.IRQ(bus);
    }

    if (cpu.waiting) {
        if ((bus.ReadDirect(ACIA_STATUS) & 0x80) != 0 || via.isIRQAsserted()) {
            cpu.waiting = false;
        } else {
            return 0;
        }
    }

    if (hasInput.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty() && (bus.ReadDirect(ACIA_STATUS) & 0x80) == 0 &&
            (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
            char c = inputBuffer.front();
            inputBuffer.pop_front();

            acia.ReceiveData(c);
            cpu.IRQ(bus);

            baudDelay = 2000;
        }
        if (inputBuffer.empty()) {
            hasInput.store(false, std::memory_order_relaxed);
        }
    }
    return res;
}

void Emulator::InjectKey(char c) {
    if (c == '\n') c = '\r';
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(c);
    hasInput.store(true, std::memory_order_relaxed);
}

void Emulator::SetOutputCallback(std::function<void(char)> cb) {
    acia.SetOutputCallback(cb);
}
void Emulator::Start() {
    if (running) return;
    running = true;
    paused = true;
    emulatorThread = std::thread(&Emulator::ThreadLoop, this);
}

void Emulator::Stop() {
    running = false;
    Resume();
    if (emulatorThread.joinable()) {
        emulatorThread.join();
    }
}

void Emulator::Pause() {
    paused = true;
    pauseCV.notify_all();
}

void Emulator::Resume() {
    paused = false;
    pauseCV.notify_all();
}

void Emulator::ThreadLoop() {
    using namespace std::chrono;
    auto nextSliceTime = high_resolution_clock::now();
    int instructionsThisSecond = 0;
    auto lastSecondTime = high_resolution_clock::now();

    double instructionAccumulator = 0.0;
    auto lastWatchCheck = high_resolution_clock::now();

    while (running) {
        {
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
        }
        if (!running) break;

        int currentTarget = targetIPS.load();
        if (currentTarget <= 0) currentTarget = 1;

        int sliceDurationMs = 10;
        double targetPerSlice =
            (double)currentTarget / (1000.0 / sliceDurationMs);

        instructionAccumulator += targetPerSlice;
        int instructionsPerSlice = (int)instructionAccumulator;
        instructionAccumulator -= instructionsPerSlice;

        auto sliceStartTime = high_resolution_clock::now();

        {
            std::lock_guard<std::mutex> lock(emulationMutex);
            for (int i = 0; i < instructionsPerSlice; ++i) {
                int res = Step();
                if (res != 0) {
                    paused = true;
                    std::cerr << "Emulator stopped with code: " << res
                              << std::endl;
                    break;
                }
            }
        }

        instructionsThisSecond += instructionsPerSlice;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            actualIPS = instructionsThisSecond;
            instructionsThisSecond = 0;
            lastSecondTime = now;
        }

        nextSliceTime += milliseconds(sliceDurationMs);

        std::this_thread::sleep_until(nextSliceTime);

        if (high_resolution_clock::now() > nextSliceTime) {
            nextSliceTime = high_resolution_clock::now();
        }

        if (autoReloadRequested.load() && !currentBinPath.empty()) {
            auto now = high_resolution_clock::now();
            if (duration_cast<milliseconds>(now - lastWatchCheck).count() >=
                500) {
                lastWatchCheck = now;
                try {
                    if (std::filesystem::exists(currentBinPath)) {
                        auto latestTime =
                            std::filesystem::last_write_time(currentBinPath);
                        if (latestTime > lastBinModificationTime) {
                            std::string errorMsg;
                            std::cerr << "Auto-reloading: " << currentBinPath
                                      << std::endl;
                            if (Init(currentBinPath, errorMsg)) {
                                Console::Clear();
                            } else {
                                std::cerr << "Auto-reload failed: " << errorMsg
                                          << std::endl;
                            }
                        }
                    }
                } catch (...) {
                    // Ignore filesystem errors
                }
            }
        }
    }
}

}  // namespace Core
