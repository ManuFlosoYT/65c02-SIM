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
    void Step();
    void InjectKey(char c);
    void SetOutputCallback(std::function<void(char)> cb);

    // Getters for GUI/Debug
    const CPU& GetCPU() const { return cpu; }
    const Mem& GetMem() const { return mem; }

    void PrintState();  // Keep for legacy terminal

private:
    // Componentes
    Mem mem;
    CPU cpu;
    LCD lcd;
    ACIA acia;

    // Buffer de entrada
    std::deque<char> inputBuffer;
    int baudDelay = 0;
};
