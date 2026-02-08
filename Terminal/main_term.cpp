#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "../Componentes/Emulator.h"

static struct termios orig_termios;

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

bool paused = false;
bool running = true;
bool debugTrace = false;
int inputState = 0;
Emulator emulator;

void PrintState() { 
    emulator.PrintState(); 
}

void HandleInput() {
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
                             "Quit, 'i' Input 1 char, 'Enter' Newline+Step\r"
                          << std::endl;
                emulator.PrintState();
                continue;
            case 3:  // Ctrl-C
                reset_terminal_mode();
                std::cout << "\n--- Execution interrupted ---" << std::endl;
                exit(0);
            default:
                break;
        }

        if (paused) {
            // Special handling when paused
            switch (c) {
                case 's':
                case ' ':  // Step
                    emulator.Step();
                    emulator.PrintState();
                    break;
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
                    emulator.InjectKey(input);
                    std::cerr << "\r\nInjected '"
                              << (char)(isprint(input) ? input : '?') << "' ("
                              << std::hex << std::uppercase << std::setw(2)
                              << std::setfill('0') << (int)(unsigned char)input
                              << ").\r\n"
                              << std::dec;
                    break;
                }
                case '\n':
                case '\r':  // Newline + Step
                    emulator.InjectKey('\r');
                    emulator.Step();
                    std::cerr << "Intro injected.\r" << std::endl;
                    emulator.PrintState();
                    break;
                default:
                    break;
            }
            continue;
        }

        // Normal input handling (passed to emulator via InjectKey)
        switch (inputState) {
            case 0:
                if (c == 0x1B) {
                    inputState = 1;  // Got ESC
                } else {
                    emulator.InjectKey((char)c);
                }
                break;
            case 1:
                if (c == '[') {
                    inputState = 2;  // Got CSI
                } else {
                    emulator.InjectKey(0x1B);
                    emulator.InjectKey((char)c);
                    inputState = 0;
                }
                break;
            case 2:
                if (c >= 0x40 && c <= 0x7E) {
                    inputState = 0;
                }
                break;
            default:
                break;
        }
    }
}

// eater.bin is hardcoded to be an interactive program
// TODO: Remove this hardcoding
int main(int argc, char* argv[]) {

    std::string bin;
    if (argc > 1) {
        bin = argv[1];
    } else {
        std::cout << "Enter the name of the binary without extension:";
        std::cin >> bin;
    }

    bool interactive = false;
    if (bin == "eater" || bin.find("eater.bin") != std::string::npos) {
        interactive = true;
    }

    emulator.Init(bin);
    emulator.SetOutputCallback([](char c) {
        std::cout << c;
        std::cout.flush();
    });

    if (interactive) {
        set_conio_terminal_mode();
        std::cout << "Interactive Mode. Press Ctrl-C to exit.\r" << std::endl;
    }

    std::cout << "--- Starting program ---\r" << std::endl;

    while (running) {
        if (paused) {
            HandleInput();  // Wait for command
            usleep(10000);
            continue;
        }

        emulator.Step();

        if (interactive) {
            HandleInput();
        }

        if (debugTrace) {
            emulator.PrintState();
        }
    }

    if (interactive) reset_terminal_mode();
    std::cout << std::endl;
    std::cout << "--- Execution finished ---" << std::endl;

    return 0;
}
