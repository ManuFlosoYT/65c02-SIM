#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "Componentes/ACIA.h"
#include "Componentes/CPU.h"
#include "Componentes/LCD.h"
#include "Componentes/Mem.h"

struct termios orig_termios;

void reset_terminal_mode() { tcsetattr(0, TCSANOW, &orig_termios); }

void set_conio_terminal_mode() {
    struct termios new_termios;

    // take two copies - one for now, one for later
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    // register cleanup handler, and set the new terminal mode
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

void CargarBinario(Mem& mem, const std::string& bin) {
    std::string ruta = "Programas/build/" + bin + ".bin";

    FILE* fichero = fopen(ruta.c_str(), "rb");
    if (fichero == nullptr) {
        std::cerr << "Error al abrir el archivo " << ruta << std::endl;
        return;
    }

    // asegurar que el bin tiene tamaño correcto
    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != mem.ROM_SIZE) {
        std::cerr << "Error: El archivo " << ruta << " no tiene tamaño "
                  << mem.ROM_SIZE << std::endl;
        return;
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, mem.ROM_SIZE, fichero);
    if (bytesRead == 0) {
        std::cerr << "Error al leer el archivo " << ruta << std::endl;
        return;
    }

    fclose(fichero);
}

int main(int argc, char* argv[]) {
    Mem mem{};
    CPU cpu{};
    LCD lcd;
    ACIA acia;

    cpu.Reset(mem);
    lcd.Inicializar(mem);
    acia.Inicializar(mem);

    std::string bin;
    if (argc > 1) {
        bin = argv[1];
    } else {
        std::cout << "Introduce el nombre del binario sin extension:";
        std::cin >> bin;
    }

    CargarBinario(mem, bin);

    std::cout << "--- Lanzando programa ---\n";

    // Modo interactivo para Wozmon y BASIC
    bool interactive = true;
    if (interactive) {
        set_conio_terminal_mode();
        std::cout << "Interactive Mode. Press Ctrl-C to exit.\r\n";
    }

    // Debug trace state
    static bool debugTrace = false;
    static bool paused = false;

    while (true) {
        if (paused) {
            if (kbhit()) {
                char c = getchar();
                if (c == 's' || c == ' ') {  // Step
                    int res = cpu.Step(mem);
                    if (res != 0) break;
                    fprintf(stderr,
                            "PC: %04X A:%02X X:%02X Y:%02X SP:%04X P:%02X "
                            "OP:%02X\r\n",
                            cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP,
                            cpu.GetStatus(), mem.memoria[cpu.PC]);
                } else if (c == 'c') {  // Continue
                    paused = false;
                    fprintf(stderr, "Resuming...\r\n");
                } else if (c == 'q') {  // Quit
                    break;
                } else if (c == 'i') {  // Inject input
                    fprintf(stderr, "Character to inject: ");
                    // Simple blocking wait for next char
                    while (!kbhit()) usleep(1000);
                    char input = getchar();

                    mem.memoria[ACIA_DATA] = input;
                    mem.memoria[ACIA_STATUS] |= 0x80;
                    // Trigger IRQ logic (simulation)
                    cpu.IRQ(mem);
                    fprintf(stderr,
                            "\r\nInjected '%c' (%02X). IRQ Triggered.\r\n",
                            (isprint(input) ? input : '?'), input);
                } else if (c == '\n' || c == '\r') {  // Newline + Step
                    mem.memoria[ACIA_DATA] = '\r';
                    mem.memoria[ACIA_STATUS] |= 0x80;
                    cpu.IRQ(mem);

                    int res = cpu.Step(mem);
                    if (res != 0) break;
                    fprintf(stderr, "Intro injected.\r\n");
                    fprintf(stderr,
                            "PC: %04X A:%02X X:%02X Y:%02X SP:%04X P:%02X "
                            "OP:%02X\r\n",
                            cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP,
                            cpu.GetStatus(), mem.memoria[cpu.PC]);
                }
            } else {
                usleep(10000);  // Sleep to avoid CPU hogging
            }
            continue;  // Skip normal execution
        }

        // Normal execution
        int res = cpu.Step(mem);
        if (res != 0) break;

        if ((mem.memoria[ACIA_STATUS] & 0x80) != 0) {
            cpu.IRQ(mem);
        }

        if (interactive && kbhit()) {
            char c = getchar();
            if (c == 4) {  // Ctrl-D toggle debug trace
                debugTrace = !debugTrace;
                fprintf(stderr, "Debug Trace: %s\r\n",
                        debugTrace ? "ON" : "OFF");
                continue;
            }
            if (c == 5) {  // Ctrl-E toggle Pause
                paused = true;
                fprintf(
                    stderr,
                    "\n\rPAUSED. Keys: 's' Step, 'c' Continue, 'q' Quit, 'i' "
                    "Input 1 char, 'Enter' Newline+Step\r\n");
                fprintf(
                    stderr,
                    "PC: %04X A:%02X X:%02X Y:%02X SP:%04X P:%02X OP:%02X\r\n",
                    cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.GetStatus(),
                    mem.memoria[cpu.PC]);
                continue;
            }
            if (c == 3) break;  // Ctrl-C
            //if (c == '\n') c = '\r';

            // Simular ACIA: Escribir dato y activar IRQ
            mem.memoria[ACIA_DATA] = c;
            mem.memoria[ACIA_STATUS] |=
                0x80;  // Bit 7: Data Register Full / IRQ

            cpu.IRQ(mem);
        }

        if (debugTrace) {
            fprintf(stderr,
                    "PC: %04X A:%02X X:%02X Y:%02X SP:%04X P:%02X OP:%02X\r\n",
                    cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.GetStatus(),
                    mem.memoria[cpu.PC]);
        }
    }

    if (interactive) reset_terminal_mode();

    std::cout << "\n--- Ejecucion finalizada ---\n";
}