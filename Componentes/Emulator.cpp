#include "Emulator.h"

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>

Emulator::Emulator() : mem(), cpu(), lcd(), acia() {}

static struct termios orig_termios;

void Emulator::PrintState() {
    std::cerr << "PC: 0x" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << (int)cpu.PC << " OP: 0x" << std::setw(2)
              << (int)mem.memoria[cpu.PC] << " SP: 0x" << std::setw(4)
              << (int)cpu.SP << " A: 0x" << std::setw(2) << (int)cpu.A
              << " X: 0x" << std::setw(2) << (int)cpu.X << " Y: 0x"
              << std::setw(2) << (int)cpu.Y << " Flags: 0x" << std::setw(2)
              << (int)cpu.GetStatus() << "\r" << std::dec << std::endl;
}

static void reset_terminal_mode() { 
    tcsetattr(0, TCSANOW, &orig_termios); 
}

static void set_conio_terminal_mode() {
    struct termios new_termios;

    // take two copies - one for now, one for later
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    // register cleanup handler, and set the new terminal mode
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

static int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
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
        std::cerr << "Error al abrir el archivo " << ruta << std::endl;
        exit(-1);
    }

    // asegurar que el bin tiene tamaño correcto
    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != mem.ROM_SIZE) {
        std::cerr << "Error: El archivo " << ruta << " no tiene tamaño "
                  << mem.ROM_SIZE << std::endl;
        fclose(fichero);
        exit(-2);
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, mem.ROM_SIZE, fichero);
    if (bytesRead == 0) {
        std::cerr << "Error al leer el archivo " << ruta << std::endl;
        fclose(fichero);
        exit(-3);
    }

    fclose(fichero);

    if (bin == "basic") {
        interactive = true;
    }

    if (interactive) {
        set_conio_terminal_mode();
        std::cout << "Interactive Mode. Press Ctrl-C to exit.\r" << std::endl;
    }
}

void Emulator::Run() {
    std::cout << "--- Lanzando programa ---\r" << std::endl;

    while (running) {
        StepLoop();
    }

    if (interactive) reset_terminal_mode();
    std::cout << std::endl;
    std::cout << "--- Ejecucion finalizada ---" << std::endl;
}

void Emulator::StepLoop() {
    if (paused) {
        HandleDebug();
        return;
    }

    // Normal execution
    int res = cpu.Step(mem);
    if (res != 0) {
        running = false;
        return;
    }

    if (baudDelay > 0) baudDelay--;

    if ((mem.memoria[ACIA_STATUS] & 0x80) != 0) {
        cpu.IRQ(mem);
    }

    if (interactive) {
        HandleInput();
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

    if (debugTrace) {
        PrintState();
    }
}

void Emulator::HandleDebug() {
    if (!kbhit()) {
        usleep(10000);
        return;
    }

    char c = getchar();
    switch (c) {
        case 's':
        case ' ':  // Step
        {
            if (cpu.Step(mem) != 0) {
                running = false;
            }
            PrintState();
            break;
        }
        case 'c':  // Continue
            paused = false;
            std::cerr << "Resuming...\r" << std::endl;
            break;
        case 'q':  // Quit
            running = false;
            break;
        case 'i':  // Inject input
        {
            std::cerr << "Character to inject: ";
            // Simple blocking wait for next char
            while (!kbhit()) usleep(1000);
            char input = getchar();

            mem.memoria[ACIA_DATA] = input;
            mem.memoria[ACIA_STATUS] |= 0x80;
            cpu.IRQ(mem);

            std::cerr << "\r\nInjected '"
                      << (char)(isprint(input) ? input : '?') << "' ("
                      << std::hex << std::uppercase << std::setw(2)
                      << std::setfill('0') << (int)(unsigned char)input
                      << "). IRQ Triggered.\r\n"
                      << std::dec;
            break;
        }
        case '\n':
        case '\r':  // Newline + Step
        {
            mem.memoria[ACIA_DATA] = '\r';
            mem.memoria[ACIA_STATUS] |= 0x80;
            cpu.IRQ(mem);

            if (cpu.Step(mem) != 0) {
                running = false;
            }
            std::cerr << "Intro injected.\r" << std::endl;
            PrintState();
            break;
        }
        default:
            break;
    }
}

void Emulator::HandleInput() {
    if (inputPollDelay > 0) {
        inputPollDelay--;
        return;
    }

    inputPollDelay = 1000;  // Check input every 1000 instructions

    while (kbhit() > 0) {
        int c = getchar();
        switch (c) {
            case EOF:
                break;
            case 4:  // Ctrl-D toggle debug trace
                debugTrace = !debugTrace;
                std::cerr << "Debug Trace: " << (debugTrace ? "ON" : "OFF")
                          << "\r" << std::endl;
                continue;
            case 5:  // Ctrl-E toggle Pause
                paused = true;
                std::cerr << "\n\rPAUSED. Keys: 's' Step, 'c' Continue, 'q' "
                             "Quit, 'i' "
                             "Input 1 char, 'Enter' Newline+Step\r"
                          << std::endl;
                PrintState();
                continue;
            case 3:  // Ctrl-C
                reset_terminal_mode();
                std::cout << "\n--- Ejecucion interrumpida ---" << std::endl;
                exit(0);
            default:
                break;
        }

        switch (inputState) {
            case 0:
                if (c == 0x1B) {
                    inputState = 1;  // Got ESC
                } else {
                    if (c == '\n') c = '\r';
                    inputBuffer.push_back(c);
                }
                break;
            case 1:
                if (c == '[') {
                    inputState = 2;  // Got CSI
                } else {
                    // Not a CSI sequence, push buffered ESC and current char
                    inputBuffer.push_back(0x1B);
                    if (c == '\n') c = '\r';
                    inputBuffer.push_back(c);
                    inputState = 0;
                }
                break;
            case 2:
                if (c >= 0x40 && c <= 0x7E) {
                    inputState = 0;  // Sequence ended (and dropped)
                }
                break;
            default:
                break;
        }
    }
}
