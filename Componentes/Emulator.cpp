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
    // Normal execution
    int res = cpu.Step(mem);

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
    return res;
}

void Emulator::InjectKey(char c) {
    if (c == '\n') c = '\r';
    inputBuffer.push_back(c);
}

void Emulator::SetOutputCallback(std::function<void(char)> cb) {
    acia.SetOutputCallback(cb);
}
