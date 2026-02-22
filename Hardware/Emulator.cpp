#include "Emulator.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Frontend/Control/Console.h"
#include "Libs/picosha2.h"

using namespace Hardware;

namespace Core {

Emulator::Emulator() : mem(), cpu(), lcd(), acia(), via() {}

void Emulator::PrintState() {
    std::cerr << "PC: 0x" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << (int)cpu.PC << " OP: 0x" << std::setw(2)
              << (int)mem.memory[cpu.PC] << " SP: 0x" << std::setw(4)
              << (int)cpu.SP << " A: 0x" << std::setw(2) << (int)cpu.A
              << " X: 0x" << std::setw(2) << (int)cpu.X << " Y: 0x"
              << std::setw(2) << (int)cpu.Y << " Flags: 0x" << std::setw(2)
              << (int)cpu.GetStatus() << "\r" << std::dec << std::endl;
}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(emulationMutex);
    cpu.Reset(mem);
    lcd.Init(mem);  // Clears state
    acia.Init(mem);
    via.Init(mem);

    // Connect VIA Port B Output to LCD Input
    via.SetPortBCallback([this](Byte val) { lcd.Update(val); });

    // Set up GPU VRAM write hook for RAM addresses 0x2000-0x3FFF (3 MSB = 001)
    for (Word addr = 0x2000; addr < 0x4000; addr++) {
        mem.SetWriteHook(
            addr,
            [](void* context, Word a, Byte val) {
                auto self = static_cast<Emulator*>(context);
                self->mem.memory[a] = val;
                if (self->gpuEnabled) {
                    self->gpu.Write(a - 0x2000, val);
                }
            },
            this);
    }

    // Reset SID state
    sid.Reset();

    // Map SID to 0x4800 - 0x481F
    for (Word addr = 0x4800; addr <= 0x481F; addr++) {
        mem.SetWriteHook(
            addr,
            [](void* context, Word a, Byte val) {
                auto self = static_cast<Emulator*>(context);
                self->sid.Write(a - 0x4800, val);
                self->mem.memory[a] = val;
            },
            this);

        mem.SetReadHook(
            addr,
            [](void* context, Word a) {
                return static_cast<Emulator*>(context)->sid.Read(a - 0x4800);
            },
            this);
    }

    std::string path = bin;

    // Try opening as is
    FILE* file = fopen(path.c_str(), "rb");

    // If fail, try appending .bin
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

    // ensure bin has correct size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize != mem.ROM_SIZE) {
        errorMsg = "Error: The file " + path + " does not have size " +
                   std::to_string(mem.ROM_SIZE);
        fclose(file);
        return false;
    }
    fseek(file, 0, SEEK_SET);

    // read bin into memory (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memory + 0x8000, 1, mem.ROM_SIZE, file);
    if (bytesRead == 0) {
        errorMsg = "Error reading file " + path;
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

bool Emulator::SaveState(const std::string& filename) {
    std::lock_guard<std::mutex> lock(emulationMutex);

    std::stringstream ss;

    // Save state of all components
    if (!mem.SaveState(ss)) return false;
    if (!cpu.SaveState(ss)) return false;
    if (!via.SaveState(ss)) return false;
    if (!sid.SaveState(ss)) return false;
    if (!acia.SaveState(ss)) return false;
    if (!lcd.SaveState(ss)) return false;
    if (!gpu.SaveState(ss)) return false;
    if (!Console::SaveState(ss)) return false;

    std::string payload = ss.str();

    // Compute SHA256 hash
    std::string hashHex = picosha2::hash256_hex_string(payload);

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;

    // Write magic header
    const char magic[] = "SIM65C02SST";
    out.write(magic, sizeof(magic) - 1);  // Exclude null terminator

    // Write payload
    out.write(payload.c_str(), payload.size());

    // Write hash
    out.write(hashHex.c_str(), hashHex.size());

    return out.good();
}

bool Emulator::LoadState(const std::string& filename, bool ignoreHash) {
    std::lock_guard<std::mutex> lock(emulationMutex);

    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    const char magic[] = "SIM65C02SST";
    size_t magicLen = sizeof(magic) - 1;
    size_t hashLen = 64;  // SHA256 hex string length is 64

    if (size < (std::streamsize)(magicLen + hashLen)) return false;

    // Verify magic header
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

    // Verify hash
    if (!ignoreHash) {
        std::string computedHash = picosha2::hash256_hex_string(payload);
        if (strncmp(fileHash, computedHash.c_str(), hashLen) != 0) {
            std::cerr << "Save state hash mismatch! Computed: " << computedHash
                      << " Expected: " << std::string(fileHash, hashLen)
                      << std::endl;
            return false;
        }
    }

    std::stringstream ss(payload);

    if (!mem.LoadState(ss)) return false;
    if (!cpu.LoadState(ss)) return false;
    if (!via.LoadState(ss)) return false;
    if (!sid.LoadState(ss)) return false;
    if (!acia.LoadState(ss)) return false;
    if (!lcd.LoadState(ss)) return false;
    if (!gpu.LoadState(ss)) return false;
    if (!Console::LoadState(ss)) return false;

    // Sync VRAM to recreate Write Hooks
    for (Word addr = 0x2000; addr < 0x4000; addr++) {
        gpu.Write(addr - 0x2000, mem.memory[addr]);
    }

    return ss.good() || ss.eof();
}

int Emulator::Step() {
    int res = 0;

    // Clock the GPU first to advance pixel counters
    if (gpuEnabled) {
        gpu.Clock();

        // CPU only executes during blanking interval
        // During drawing time, GPU has control of the bus
        if (gpu.IsInBlankingInterval()) {
            res = cpu.Step(mem);
        }
    } else {
        // If GPU is disabled, CPU runs normally
        // PrintState(); // Debugging
        res = cpu.Step(mem);
        // PrintState();
        // if (cpu.PC > 0x8000) PrintState();  // Log only ROM execution
    }

    if (baudDelay > 0) baudDelay--;

    // Check ACIA IRQ
    if ((mem.memory[ACIA_STATUS] & 0x80) != 0) {
        cpu.IRQ(mem);
    }

    // Clock VIA and check IRQ
    via.Clock();
    if (via.isIRQAsserted()) {
        cpu.IRQ(mem);
    }

    // If waiting (WAI), check if there is a pending interrupt
    if (cpu.waiting) {
        if ((mem.memory[ACIA_STATUS] & 0x80) != 0 || via.isIRQAsserted()) {
            cpu.waiting = false;
        } else {
            return 0;
        }
    }

    // Process input buffer if ACIA is ready
    if (hasInput.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty() && (mem.memory[ACIA_STATUS] & 0x80) == 0 &&
            (via.GetPortA() & 0x01) == 0 && baudDelay <= 0) {
            char c = inputBuffer.front();
            inputBuffer.pop_front();

            // Simulate ACIA: Write data and trigger IRQ
            mem.memory[ACIA_DATA] = c;
            mem.memory[ACIA_STATUS] |= 0x80;  // Bit 7: Data Register Full / IRQ
            cpu.IRQ(mem);

            // Set delay (2000 clock cycles)
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
    paused = true;  // Start paused
    emulatorThread = std::thread(&Emulator::ThreadLoop, this);
}

void Emulator::Stop() {
    running = false;
    Resume();  // Unpause to let thread finish
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

    while (running) {
        {
            std::unique_lock<std::mutex> lock(threadMutex);
            pauseCV.wait(lock, [this] { return !paused || !running; });
        }
        if (!running) break;

        int currentTarget = targetIPS.load();
        if (currentTarget <= 0) currentTarget = 1;

        // Execute in 10ms slices
        int sliceDurationMs = 10;
        double targetPerSlice =
            (double)currentTarget / (1000.0 / sliceDurationMs);

        instructionAccumulator += targetPerSlice;
        int instructionsPerSlice = (int)instructionAccumulator;
        instructionAccumulator -= instructionsPerSlice;

        auto sliceStartTime = high_resolution_clock::now();

        // Lock emulation mutex for the entire slice to prevent race with
        // Reset/Init
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

        // Sleep until next slice time
        std::this_thread::sleep_until(nextSliceTime);

        // If we are lagging behind (processing took longer than slice
        // duration), reset the schedule to avoid running too fast to catch up.
        if (high_resolution_clock::now() > nextSliceTime) {
            nextSliceTime = high_resolution_clock::now();
        }
    }
}

}  // namespace Core
