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

void Emulator::Init(const std::string& bin) {
    cpu.Reset(mem);
    lcd.Inicializar(mem);
    acia.Inicializar(mem);

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
        std::cerr << "Error opening file " << ruta << std::endl;
        exit(-1);
    }

    // asegurar que el bin tiene tamaño correcto
    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != mem.ROM_SIZE) {
        std::cerr << "Error: The file " << ruta << " does not have size "
                  << mem.ROM_SIZE << std::endl;
        fclose(fichero);
        exit(-2);
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, mem.ROM_SIZE, fichero);
    if (bytesRead == 0) {
        std::cerr << "Error reading file " << ruta << std::endl;
        fclose(fichero);
        exit(-3);
    }

    fclose(fichero);
}

void Emulator::Step() {
    // Normal execution
    cpu.Step(mem);

    if (baudDelay > 0) baudDelay--;

    if ((mem.memoria[ACIA_STATUS] & 0x80) != 0) {
        cpu.IRQ(mem);
    }

    // Procesar input buffer si ACIA está listo
    if (!inputBuffer.empty() && (mem.memoria[ACIA_STATUS] & 0x80) == 0 &&
        (mem.memoria[PORTA] & 0x01) == 0 && baudDelay <= 0) {
        char c = inputBuffer.front();
        inputBuffer.pop_front();

        // Simular ACIA: Escribir dato y activar IRQ
        mem.memoria[ACIA_DATA] = c;
        mem.memoria[ACIA_STATUS] |= 0x80;  // Bit 7: Data Register Full / IRQ
        cpu.IRQ(mem);

        // Set delay (2000 ciclos de reloj)
        baudDelay = 2000;
    }
}

void Emulator::InjectKey(char c) {
    if (c == '\n') c = '\r';
    inputBuffer.push_back(c);
}

void Emulator::SetOutputCallback(std::function<void(char)> cb) {
    acia.SetOutputCallback(cb);
}
