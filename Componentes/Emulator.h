#include <deque>
#include <string>

#include "ACIA.h"
#include "CPU.h"
#include "LCD.h"
#include "Mem.h"

class Emulator {
public:
    Emulator();
    ~Emulator() = default;

    void Init(const std::string& binPath);
    void Run();

private:
    void StepLoop();
    void HandleInput();
    void HandleDebug();
    void PrintState();

    // Componentes
    Mem mem;
    CPU cpu;
    LCD lcd;
    ACIA acia;

    // Estado
    bool running = true;
    bool paused = false;
    bool debugTrace = false;
    bool interactive = false;

    // Buffer de entrada
    std::deque<char> inputBuffer;
    int baudDelay = 0;
    int inputPollDelay = 0;
    int inputState = 0;
};
