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
    SetupHardware();

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
    if (!cpu.SaveState(ss)) return false;
    if (!ram.SaveState(ss)) return false;
    if (!rom.SaveState(ss)) return false;
    if (!via.SaveState(ss)) return false;
    if (!sid.SaveState(ss)) return false;
    if (!acia.SaveState(ss)) return false;
    if (!lcd.SaveState(ss)) return false;
    if (!gpu.SaveState(ss)) return false;
    if (!Console::SaveState(ss)) return false;

    // Internal Emulator state
    ss.write(reinterpret_cast<const char*>(&baudDelay), sizeof(baudDelay));

    bool gpuE = gpuEnabled;
    ss.write(reinterpret_cast<const char*>(&gpuE), sizeof(gpuE));

    int tIPS = targetIPS.load();
    ss.write(reinterpret_cast<const char*>(&tIPS), sizeof(tIPS));

    size_t qSize = inputBuffer.size();
    ss.write(reinterpret_cast<const char*>(&qSize), sizeof(qSize));
    for (char c : inputBuffer) {
        ss.write(&c, 1);
    }

    size_t binPathLen = currentBinPath.length();
    ss.write(reinterpret_cast<const char*>(&binPathLen), sizeof(binPathLen));
    ss.write(currentBinPath.c_str(), binPathLen);

    bool autoReload = autoReloadRequested.load();
    ss.write(reinterpret_cast<const char*>(&autoReload), sizeof(autoReload));

    std::string payload = ss.str();
    std::string payloadHash = picosha2::hash256_hex_string(payload);

    std::string version = PROJECT_VERSION;
    uint32_t versionLen = static_cast<uint32_t>(version.length());
    const char magic[] = "SIM65C02SST";

    // Metadata hash (Magic + Version)
    std::string metadata = std::string(magic) + version;
    std::string metadataHash = picosha2::hash256_hex_string(metadata);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(magic, sizeof(magic) - 1);
    out.write(reinterpret_cast<const char*>(&versionLen), sizeof(versionLen));
    out.write(version.c_str(), versionLen);
    out.write(payload.c_str(), payload.size());
    out.write(payloadHash.c_str(), payloadHash.size());
    out.write(metadataHash.c_str(), metadataHash.size());

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool forceLoad) {
    std::lock_guard<std::mutex> lock(emulationMutex);
    SetupHardware();

    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    const char magic[] = "SIM65C02SST";
    size_t magicLen = sizeof(magic) - 1;
    size_t hashLen = 64;

    if (size < (std::streamsize)(magicLen + sizeof(uint32_t) + (hashLen * 2)))
        return false;

    char fileMagic[16] = {0};
    in.read(fileMagic, magicLen);
    if (strncmp(fileMagic, magic, magicLen) != 0) {
        return false;
    }

    uint32_t versionLen = 0;
    in.read(reinterpret_cast<char*>(&versionLen), sizeof(versionLen));
    lastLoadVersion.assign(versionLen, '\0');
    in.read(&lastLoadVersion[0], versionLen);

    // Calculate metadata hash for verification (Magic + Version)
    std::string metadata = std::string(magic) + lastLoadVersion;
    std::string computedMetadataHash = picosha2::hash256_hex_string(metadata);

    size_t payloadSize =
        size - (magicLen + sizeof(versionLen) + versionLen + (hashLen * 2));
    std::string payload(payloadSize, '\0');
    in.read(&payload[0], payloadSize);

    char filePayloadHash[64];
    in.read(filePayloadHash, hashLen);

    char fileMetadataHash[64];
    in.read(fileMetadataHash, hashLen);

    lastLoadResult = SavestateLoadResult::Success;

    // 1. Verify Metadata Hash
    if (strncmp(fileMetadataHash, computedMetadataHash.c_str(), hashLen) != 0) {
        std::cerr << "Error: Savestate metadata corruption detected!"
                  << std::endl;
        lastLoadResult = SavestateLoadResult::GenericError;
        if (!forceLoad) return false;
    }

    // 2. Verify Payload Hash
    std::string computedPayloadHash = picosha2::hash256_hex_string(payload);
    if (strncmp(filePayloadHash, computedPayloadHash.c_str(), hashLen) != 0) {
        lastLoadResult = SavestateLoadResult::HashMismatch;
        std::cerr << "Warning: Savestate payload hash mismatch!" << std::endl;
    }

    // 3. Check Version
    if (lastLoadVersion != PROJECT_VERSION) {
        if (lastLoadResult == SavestateLoadResult::Success) {
            lastLoadResult = SavestateLoadResult::VersionMismatch;
        }
        std::cerr << "Warning: Savestate version mismatch! File: "
                  << lastLoadVersion << " Current: " << PROJECT_VERSION
                  << std::endl;
    }

    std::stringstream ss(payload);

    // Load components
    bool structuralSuccess = true;
    if (!bus.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !cpu.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !ram.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !rom.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !via.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !sid.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !acia.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !lcd.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !gpu.LoadState(ss)) structuralSuccess = false;
    if (structuralSuccess && !Console::LoadState(ss)) structuralSuccess = false;

    if (structuralSuccess) {
        ss.read(reinterpret_cast<char*>(&baudDelay), sizeof(baudDelay));

        bool gpuE;
        ss.read(reinterpret_cast<char*>(&gpuE), sizeof(gpuE));
        gpuEnabled = gpuE;

        int tIPS;
        ss.read(reinterpret_cast<char*>(&tIPS), sizeof(tIPS));
        targetIPS.store(tIPS);

        size_t qSize = 0;
        ss.read(reinterpret_cast<char*>(&qSize), sizeof(qSize));
        std::lock_guard<std::mutex> lock(bufferMutex);
        inputBuffer.clear();
        for (size_t i = 0; i < qSize; ++i) {
            char c;
            ss.read(&c, 1);
            inputBuffer.push_back(c);
        }
        hasInput.store(!inputBuffer.empty());

        size_t binPathLen = 0;
        ss.read(reinterpret_cast<char*>(&binPathLen), sizeof(binPathLen));
        currentBinPath.assign(binPathLen, '\0');
        ss.read(&currentBinPath[0], binPathLen);

        bool autoReload;
        ss.read(reinterpret_cast<char*>(&autoReload), sizeof(autoReload));
        autoReloadRequested.store(autoReload);
    }

    if (!structuralSuccess || (!ss.good() && !ss.eof())) {
        lastLoadResult = SavestateLoadResult::StructuralError;
        if (!forceLoad) return false;
    }

    return true;
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

void Emulator::SetupHardware() {
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
}

}  // namespace Core
