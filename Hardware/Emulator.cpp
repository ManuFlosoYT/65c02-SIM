#include "Emulator.h"

#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>

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
    // The hook will write to VRAM when CPU writes to these addresses
    for (Word addr = 0x2000; addr < 0x4000; addr++) {
        mem.SetWriteHook(addr, [this, addr](Word, Byte val) {
            this->mem.memory[addr] = val;

            // Only write to VRAM if GPU is enabled
            if (gpuEnabled) {
                // Calculate VRAM address by subtracting base address
                Word vramAddr = addr - 0x2000;
                gpu.Write(vramAddr, val);
            }
        });
    }

    // Reset SID state
    sid.Reset();

    // Map SID to 0x4800 - 0x481F
    for (Word addr = 0x4800; addr <= 0x481F; addr++) {
        mem.SetWriteHook(addr, [this](Word addr, Byte val) {
            this->sid.Write(addr - 0x4800, val);
            this->mem.memory[addr] = val;
        });
        mem.SetReadHook(addr, [this](Word addr) -> Byte {
            return this->sid.Read(addr - 0x4800);
        });
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
    if ((mem.Read(ACIA_STATUS) & 0x80) != 0) {
        cpu.IRQ(mem);
    }

    // Clock VIA and check IRQ
    via.Clock();
    if (via.isIRQAsserted()) {
        cpu.IRQ(mem);
    }

    // If waiting (WAI), check if there is a pending interrupt
    if (cpu.waiting) {
        if ((mem.Read(ACIA_STATUS) & 0x80) != 0 || via.isIRQAsserted()) {
            cpu.waiting = false;
        } else {
            return 0;
        }
    }

    // Process input buffer if ACIA is ready
    {
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
    }
    return res;
}

void Emulator::InjectKey(char c) {
    if (c == '\n') c = '\r';
    std::lock_guard<std::mutex> lock(bufferMutex);
    inputBuffer.push_back(c);
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
        int instructionsPerSlice = currentTarget / (1000 / sliceDurationMs);
        if (instructionsPerSlice < 1) instructionsPerSlice = 1;

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
                // Check if paused during execution loop for faster response
                if (paused) break;
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
