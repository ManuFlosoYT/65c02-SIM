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

Emulator::Emulator() : cpu() {}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(emulationMutex);
    SetupHardware();

    std::string path = bin;

    FILE* file = fopen(path.c_str(), "rb");  // NOLINT

    if (file == nullptr && path.find(".bin") == std::string::npos) {
        std::string tryBin = path + ".bin";
        file = fopen(tryBin.c_str(), "rb");  // NOLINT
        if (file != nullptr) {
            path = tryBin;
        }
    }
    if (file == nullptr) {
        errorMsg = "Error opening file " + path;
        return false;
    }

    (void)fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize != ROM_SIZE) {
        errorMsg = "Error: The file " + path + " does not have size " + std::to_string(ROM_SIZE);
        (void)fclose(file);  // NOLINT
        return false;
    }
    (void)fseek(file, 0, SEEK_SET);

    std::vector<Byte> buffer(ROM_SIZE);
    size_t bytesRead = fread(buffer.data(), 1, ROM_SIZE, file);
    if (bytesRead == 0) {
        errorMsg = "Error reading file " + path;
        (void)fclose(file);  // NOLINT
        return false;
    }

    for (size_t i = 0; i < bytesRead; ++i) {
        rom.WriteDirect(static_cast<Word>(i), buffer[i]);
    }

    (void)fclose(file);  // NOLINT

    currentBinPath = path;
    try {
        if (std::filesystem::exists(path)) {
            lastBinModificationTime = std::filesystem::last_write_time(path);
        }
    } catch (...) {
        // Ignore filesystem errors
        (void)0;
    }

    return true;
}

bool Emulator::SaveState(const std::string& filename) {
    std::lock_guard<std::mutex> lock(emulationMutex);

    std::stringstream stateStream;
    if (!bus.SaveState(stateStream)) {
        return false;
    }
    if (!cpu.SaveState(stateStream)) {
        return false;
    }
    if (!ram.SaveState(stateStream)) {
        return false;
    }
    if (!rom.SaveState(stateStream)) {
        return false;
    }
    if (!via.SaveState(stateStream)) {
        return false;
    }
    if (!sid.SaveState(stateStream)) {
        return false;
    }
    if (!acia.SaveState(stateStream)) {
        return false;
    }
    if (!lcd.SaveState(stateStream)) {
        return false;
    }
    if (!gpu.SaveState(stateStream)) {
        return false;
    }
    if (!Console::SaveState(stateStream)) {
        return false;
    }

    // Internal Emulator state
    stateStream.write(reinterpret_cast<const char*>(&baudDelay), sizeof(baudDelay));  // NOLINT

    bool gpuE = gpuEnabled;
    stateStream.write(reinterpret_cast<const char*>(&gpuE), sizeof(gpuE));  // NOLINT

    int tIPS = targetIPS.load();
    stateStream.write(reinterpret_cast<const char*>(&tIPS), sizeof(tIPS));  // NOLINT

    size_t qSize = inputBuffer.size();
    stateStream.write(reinterpret_cast<const char*>(&qSize), sizeof(qSize));  // NOLINT
    for (char chr : inputBuffer) {
        stateStream.write(&chr, 1);
    }

    size_t binPathLen = currentBinPath.length();
    stateStream.write(reinterpret_cast<const char*>(&binPathLen), sizeof(binPathLen));  // NOLINT
    stateStream.write(currentBinPath.c_str(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = autoReloadRequested.load();
    stateStream.write(reinterpret_cast<const char*>(&autoReload), sizeof(autoReload));  // NOLINT

    std::string payload = stateStream.str();
    std::string payloadHash = picosha2::hash256_hex_string(payload);

    std::string version = PROJECT_VERSION;
    auto versionLen = static_cast<uint32_t>(version.length());
    std::array<char, 12> magic{"SIM65C02SST"};  // NOLINT

    // Metadata hash (Magic + Version)
    std::string metadata = std::string(magic.data()) + version;
    std::string metadataHash = picosha2::hash256_hex_string(metadata);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out.write(magic.data(), magic.size() - 1);
    out.write(reinterpret_cast<const char*>(&versionLen), sizeof(versionLen));  // NOLINT
    out.write(version.c_str(), static_cast<std::streamsize>(versionLen));
    out.write(payload.c_str(), static_cast<std::streamsize>(payload.size()));
    out.write(payloadHash.c_str(), static_cast<std::streamsize>(payloadHash.size()));
    out.write(metadataHash.c_str(), static_cast<std::streamsize>(metadataHash.size()));

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool forceLoad) {
    std::lock_guard<std::mutex> lock(emulationMutex);
    SetupHardware();

    std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        return false;
    }

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::array<char, 12> magic{"SIM65C02SST"};  // NOLINT
    size_t magicLen = magic.size() - 1;
    size_t hashLen = 64;

    if (size < (std::streamsize)(magicLen + sizeof(uint32_t) + (hashLen * 2))) {
        return false;
    }

    std::array<char, 16> fileMagic{};
    inFile.read(fileMagic.data(), static_cast<std::streamsize>(magicLen));
    if (strncmp(fileMagic.data(), magic.data(), magicLen) != 0) {
        return false;
    }

    uint32_t versionLen = 0;
    inFile.read(reinterpret_cast<char*>(&versionLen), sizeof(versionLen));  // NOLINT
    lastLoadVersion.assign(versionLen, '\0');
    inFile.read(lastLoadVersion.data(), static_cast<std::streamsize>(versionLen));

    // Calculate metadata hash for verification (Magic + Version)
    std::string metadata = std::string(magic.data()) + lastLoadVersion;
    std::string computedMetadataHash = picosha2::hash256_hex_string(metadata);

    size_t payloadSize = size - (magicLen + sizeof(versionLen) + versionLen + (hashLen * 2));
    std::string payload(payloadSize, '\0');
    inFile.read(payload.data(), static_cast<std::streamsize>(payloadSize));

    std::array<char, 64> filePayloadHash{};
    inFile.read(filePayloadHash.data(), static_cast<std::streamsize>(hashLen));

    std::array<char, 64> fileMetadataHash{};
    inFile.read(fileMetadataHash.data(), static_cast<std::streamsize>(hashLen));

    lastLoadResult = SavestateLoadResult::Success;

    // 1. Verify Metadata Hash
    if (strncmp(fileMetadataHash.data(), computedMetadataHash.c_str(), hashLen) != 0) {
        std::cerr << "Error: Savestate metadata corruption detected!\n";
        lastLoadResult = SavestateLoadResult::GenericError;
        if (!forceLoad) {
            return false;
        }
    }

    // 2. Verify Payload Hash
    std::string computedPayloadHash = picosha2::hash256_hex_string(payload);
    if (strncmp(filePayloadHash.data(), computedPayloadHash.c_str(), hashLen) != 0) {
        lastLoadResult = SavestateLoadResult::HashMismatch;
        std::cerr << "Warning: Savestate payload hash mismatch!\n";
    }

    // 3. Check Version
    if (lastLoadVersion != PROJECT_VERSION) {
        if (lastLoadResult == SavestateLoadResult::Success) {
            lastLoadResult = SavestateLoadResult::VersionMismatch;
        }
        std::cerr << "Warning: Savestate version mismatch! File: " << lastLoadVersion << " Current: " << PROJECT_VERSION
                  << '\n';
    }

    std::stringstream stateStream(payload);

    bool structuralSuccess = LoadComponentsState(stateStream);

    if (structuralSuccess) {
        LoadInternalState(stateStream);
    }

    if (!structuralSuccess || (!stateStream.good() && !stateStream.eof())) {
        lastLoadResult = SavestateLoadResult::StructuralError;
        if (!forceLoad) {
            return false;
        }
    }

    return true;
}

bool Emulator::LoadComponentsState(std::istream& stateStream) {
    if (!bus.LoadState(stateStream)) {
        return false;
    }
    if (!cpu.LoadState(stateStream)) {
        return false;
    }
    if (!ram.LoadState(stateStream)) {
        return false;
    }
    if (!rom.LoadState(stateStream)) {
        return false;
    }
    if (!via.LoadState(stateStream)) {
        return false;
    }
    if (!sid.LoadState(stateStream)) {
        return false;
    }
    if (!acia.LoadState(stateStream)) {
        return false;
    }
    if (!lcd.LoadState(stateStream)) {
        return false;
    }
    if (!gpu.LoadState(stateStream)) {
        return false;
    }
    if (!Console::LoadState(stateStream)) {
        return false;
    }
    return true;
}

void Emulator::LoadInternalState(std::istream& stateStream) {
    stateStream.read(reinterpret_cast<char*>(&baudDelay), sizeof(baudDelay));  // NOLINT

    bool gpuE = false;
    stateStream.read(reinterpret_cast<char*>(&gpuE), sizeof(gpuE));  // NOLINT
    gpuEnabled = gpuE;

    int tIPS = 0;
    stateStream.read(reinterpret_cast<char*>(&tIPS), sizeof(tIPS));  // NOLINT
    targetIPS.store(tIPS);

    size_t qSize = 0;
    stateStream.read(reinterpret_cast<char*>(&qSize), sizeof(qSize));  // NOLINT
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.clear();
    for (size_t i = 0; i < qSize; ++i) {
        char chr = '\0';
        stateStream.read(&chr, 1);
        inputBuffer.push_back(chr);
    }
    hasInput.store(!inputBuffer.empty());

    size_t binPathLen = 0;
    stateStream.read(reinterpret_cast<char*>(&binPathLen), sizeof(binPathLen));  // NOLINT
    currentBinPath.assign(binPathLen, '\0');
    stateStream.read(currentBinPath.data(), static_cast<std::streamsize>(binPathLen));

    bool autoReload = false;
    stateStream.read(reinterpret_cast<char*>(&autoReload), sizeof(autoReload));  // NOLINT
    autoReloadRequested.store(autoReload);
}

int Emulator::Step() {
    if (bus.HasActiveHooks()) {
        return Step<true>();
    }
    return Step<false>();
}

template <bool Debug>
int Emulator::Step() {
    int res = 0;

    if (gpuEnabled) {
        gpu.Clock();
        if (gpu.IsInBlankingInterval()) {
            res = cpu.Step<Debug>(bus);
        }
    } else {
        res = cpu.Step<Debug>(bus);
    }

    if (baudDelay > 0) {
        baudDelay--;
    }

    via.Clock();

    bool irq = acia.HasIRQ() || via.isIRQAsserted();
    if (irq) {
        if (cpu.I == 0) {
            cpu.IRQ<Debug>(bus);
        }
        cpu.waiting = false;
    } else if (cpu.waiting) {
        return 0;
    }

    // Check input atomically only every ~10,000 steps (roughly 0.1s at 100K IPS)
    static int inputCheckCounter = 0;
    if (++inputCheckCounter >= 10000) {
        inputCheckCounter = 0;
        if (hasInput.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(bufferMutex);
            if (!inputBuffer.empty() && !acia.HasIRQ() && (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
                char chr = inputBuffer.front();
                inputBuffer.pop_front();

                acia.ReceiveData(chr);
                cpu.IRQ<Debug>(bus);

                baudDelay = 2000;
            }
            if (inputBuffer.empty()) {
                hasInput.store(false, std::memory_order_relaxed);
            }
        }
    }
    return res;
}
template int Emulator::Step<true>();
template int Emulator::Step<false>();

void Emulator::InjectKey(char key) {
    if (key == '\n') {
        key = '\r';
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(key);
    hasInput.store(true, std::memory_order_relaxed);
}

void Emulator::SetOutputCallback(std::function<void(char)> callback) { acia.SetOutputCallback(std::move(callback)); }
void Emulator::Start() {
    if (running) {
        return;
    }
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

    while (running.load(std::memory_order_relaxed)) {
        if (paused.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
        }
        if (!running.load(std::memory_order_relaxed)) {
            break;
        }

        int currentTarget = targetIPS.load();
        if (currentTarget <= 0) {
            currentTarget = 1;
        }

        int sliceDurationMs = 10;
        double targetPerSlice = (double)currentTarget / (1000.0 / sliceDurationMs);

        instructionAccumulator += targetPerSlice;
        int instructionsPerSlice = (int)instructionAccumulator;
        instructionAccumulator -= instructionsPerSlice;

        EmulateSlice(instructionsPerSlice);

        instructionsThisSecond += instructionsPerSlice;

        auto now = high_resolution_clock::now();
        if (duration_cast<seconds>(now - lastSecondTime).count() >= 1) {
            actualIPS = instructionsThisSecond;
            instructionsThisSecond = 0;
            lastSecondTime = now;
        }

        nextSliceTime += milliseconds(sliceDurationMs);

        auto sleepCheckTime = high_resolution_clock::now();
        if (sleepCheckTime < nextSliceTime) {
            std::this_thread::sleep_until(nextSliceTime);
        } else {
            nextSliceTime = sleepCheckTime;
        }

        CheckAutoReload(lastWatchCheck);
    }
}

void Emulator::EmulateSlice(int instructionsPerSlice) {
    bool hooks = bus.HasActiveHooks();
    std::lock_guard<std::mutex> lock(emulationMutex);
    if (hooks) {
        for (int i = 0; i < instructionsPerSlice; ++i) {
            int res = Step<true>();
            if (res != 0) {
                paused = true;
                std::cerr << "Emulator stopped with code: " << res << '\n';
                break;
            }
        }
    } else {
        for (int i = 0; i < instructionsPerSlice; ++i) {
            int res = Step<false>();
            if (res != 0) {
                paused = true;
                std::cerr << "Emulator stopped with code: " << res << '\n';
                break;
            }
        }
    }
}

void Emulator::CheckAutoReload(std::chrono::high_resolution_clock::time_point& lastWatchCheck) {
    using namespace std::chrono;
    if (autoReloadRequested.load() && !currentBinPath.empty()) {
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - lastWatchCheck).count() >= 500) {
            lastWatchCheck = now;
            try {
                if (std::filesystem::exists(currentBinPath)) {
                    auto latestTime = std::filesystem::last_write_time(currentBinPath);
                    if (latestTime > lastBinModificationTime) {
                        std::string errorMsg;
                        std::cerr << "Auto-reloading: " << currentBinPath << '\n';
                        if (Init(currentBinPath, errorMsg)) {
                            Console::Clear();
                        } else {
                            std::cerr << "Auto-reload failed: " << errorMsg << '\n';
                        }
                    }
                }
            } catch (...) {
                // Ignore filesystem errors
                (void)0;
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
