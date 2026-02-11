#include "Emulator.h"

#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>

Emulator::Emulator() : mem(), cpu(), lcd(), acia() {}

void Emulator::PrintState() {
    std::cerr << "PC: 0x" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << (int)cpu.PC << " OP: 0x" << std::setw(2)
              << (int)mem.memoria[cpu.PC] << " SP: 0x" << std::setw(4)
              << (int)cpu.SP << " A: 0x" << std::setw(2) << (int)cpu.A
              << " X: 0x" << std::setw(2) << (int)cpu.X << " Y: 0x"
              << std::setw(2) << (int)cpu.Y << " Flags: 0x" << std::setw(2)
              << (int)cpu.GetStatus() << "\r" << std::dec << std::endl;
}

bool Emulator::Init(const std::string& bin, std::string& errorMsg) {
    cpu.Reset(mem);
    lcd.Inicializar(mem);
    acia.Inicializar(mem);

    // Set up GPU VRAM write hook for RAM addresses 0x2000-0x3FFF (3 MSB = 001)
    // The hook will write to VRAM when CPU writes to these addresses
    for (Word addr = 0x2000; addr < 0x4000; addr++) {
        mem.SetWriteHook(addr, [this, addr](Word, Byte val) {
            this->mem.memoria[addr] = val;

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
            this->mem.memoria[addr] = val;
        });
        mem.SetReadHook(addr, [this](Word addr) -> Byte {
            return this->sid.Read(addr - 0x4800);
        });
    }

    std::string ruta = bin;

    // Try opening as is
    FILE* fichero = fopen(ruta.c_str(), "rb");

    // If fail, try appending .bin
    if (fichero == nullptr && ruta.find(".bin") == std::string::npos) {
        std::string tryBin = ruta + ".bin";
        fichero = fopen(tryBin.c_str(), "rb");
        if (fichero != nullptr) {
            ruta = tryBin;
        }
    }
    if (fichero == nullptr) {
        errorMsg = "Error opening file " + ruta;
        return false;
    }

    // asegurar que el bin tiene tamaño correcto
    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != mem.ROM_SIZE) {
        errorMsg = "Error: The file " + ruta + " does not have size " +
                   std::to_string(mem.ROM_SIZE);
        fclose(fichero);
        return false;
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, mem.ROM_SIZE, fichero);
    if (bytesRead == 0) {
        errorMsg = "Error reading file " + ruta;
        fclose(fichero);
        return false;
    }

    fclose(fichero);
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
        res = cpu.Step(mem);
    }

    if (baudDelay > 0) baudDelay--;

    if ((mem.memoria[ACIA_STATUS] & 0x80) != 0) {
        cpu.IRQ(mem);
    }

    // Procesar input buffer si ACIA está listo
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!inputBuffer.empty() && (mem.memoria[ACIA_STATUS] & 0x80) == 0 &&
            (mem.memoria[PORTA] & 0x01) == 0 && baudDelay <= 0) {
            char c = inputBuffer.front();
            inputBuffer.pop_front();

            // Simular ACIA: Escribir dato y activar IRQ
            mem.memoria[ACIA_DATA] = c;
            mem.memoria[ACIA_STATUS] |=
                0x80;  // Bit 7: Data Register Full / IRQ
            cpu.IRQ(mem);

            // Set delay (2000 ciclos de reloj)
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

        for (int i = 0; i < instructionsPerSlice; ++i) {
            int res = Step();
            if (res != 0) {
                paused = true;
                std::cerr << "Emulator stopped with code: " << res << std::endl;
                break;
            }
            // Check if paused during execution loop for faster response
            if (paused) break;
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
